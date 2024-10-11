#include "./message.h"

#include <stddef.h> // For offsetof
#include <string.h> // For memset, memcpy
#include <stdio.h>  // For debugging purposes
#include <time.h>   // For time()
#include <stdlib.h> // For srand and rand

#include <arpa/inet.h> // For htonl, ntohl, htons, ntohs

// ANSI color definitions
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define MAGENTA "\033[35m"
#define RED "\033[31m"

// In message.c
char global_dns_ip[16]; // Definition of global DNS variable

// Function to initialize a DHCP message structure with default values
void init_dhcp_message(dhcp_message_t *msg)
{
    memset(msg, 0, sizeof(dhcp_message_t)); // Clear all fields

    msg->op = 1;    // BOOTREQUEST (client to server)
    msg->htype = 1; // Ethernet
    msg->hlen = 6;  // MAC address length
    msg->hops = 0;  // Hops (usually 0 for clients)

    // Seed the random number generator with the current time (only done once)
    srand((unsigned int)time(NULL));

    // Use a random transaction ID
    msg->xid = rand();
    msg->secs = 0;              // No seconds elapsed
    msg->flags = htons(0x8000); // Broadcast flag set
}

// Function to parse raw data into a dhcp_message_t structure
int parse_dhcp_message(const uint8_t *buffer, dhcp_message_t *msg)
{
    if (!buffer || !msg)
        return -1; // Sanity check for null pointers

    // Copy fields from the raw buffer to the structure
    memcpy(msg, buffer, sizeof(dhcp_message_t));

    // Perform any necessary byte-order conversions here (e.g., for multi-byte fields)
    msg->xid = ntohl(msg->xid); // Convert network byte order to host byte order
    msg->secs = ntohs(msg->secs);
    msg->flags = ntohs(msg->flags);
    msg->ciaddr = ntohl(msg->ciaddr);
    msg->yiaddr = ntohl(msg->yiaddr);
    msg->siaddr = ntohl(msg->siaddr);
    msg->giaddr = ntohl(msg->giaddr);

    return 0; // Success
}

const char *get_dhcp_message_type_name(uint8_t type)
{
    switch (type)
    {
    case DHCP_DISCOVER:
        return "DHCP DISCOVER";
    case DHCP_OFFER:
        return "DHCP OFFER";
    case DHCP_REQUEST:
        return "DHCP REQUEST";
    case DHCP_DECLINE:
        return "DHCP DECLINE";
    case DHCP_ACK:
        return "DHCP ACK";
    case DHCP_NAK:
        return "DHCP NAK";
    case DHCP_RELEASE:
        return "DHCP RELEASE";
    default:
        return "UNKNOWN DHCP MESSAGE TYPE";
    }
}

void parse_dhcp_options(const uint8_t *options, size_t options_length)
{
    size_t i = 0;
    while (i < options_length)
    {
        uint8_t option = options[i++];
        if (option == 255) {
            break; // End of options
        }

        uint8_t length = options[i++];
        if (i + length > options_length || option == 0) {
            // Ignore empty or malformed options (option 0 is invalid)
            break;
        }

        switch (option)
        {
        case 1: // Subnet Mask
            printf("Subnet Mask: %d.%d.%d.%d\n", options[i], options[i + 1], options[i + 2], options[i + 3]);
            break;

        case 51: // Lease Time
        {
            uint32_t lease_time = ntohl(*(uint32_t *)&options[i]);
            printf("IP Address Lease Time: %d seconds (%d hours, %d minutes)\n",
                   lease_time,
                   lease_time / 3600,
                   (lease_time % 3600) / 60);
        }
        break;

        case 53: // DHCP Message Type
            printf("DHCP Message Type: %d (%s)\n", options[i], get_dhcp_message_type_name(options[i]));
            break;

        case 54: // DHCP Server Identifier
            printf("DHCP Server Identifier: %d.%d.%d.%d\n", options[i], options[i + 1], options[i + 2], options[i + 3]);
            break;

        case 6: // DNS Server
            printf("DNS Server: %d.%d.%d.%d\n", options[i], options[i + 1], options[i + 2], options[i + 3]);
            break;

        default:
            // Ignore unhandled options without printing "Unhandled Option"
            break;
        }

        i += length; // Skip to the next option
    }
}

// Function to serialize a dhcp_message_t structure into a raw byte buffer
int build_dhcp_message(const dhcp_message_t *msg, uint8_t *buffer, size_t buffer_size)
{
    if (buffer_size < sizeof(dhcp_message_t))
        return -1; // Ensure the buffer is large enough

    // Clear buffer and copy DHCP message
    memset(buffer, 0, buffer_size);
    memcpy(buffer, msg, sizeof(dhcp_message_t));

    // Perform necessary byte-order conversions
    uint32_t *xid_ptr = (uint32_t *)(buffer + offsetof(dhcp_message_t, xid));
    *xid_ptr = htonl(msg->xid);

    uint16_t *secs_ptr = (uint16_t *)(buffer + offsetof(dhcp_message_t, secs));
    *secs_ptr = htons(msg->secs);

    uint16_t *flags_ptr = (uint16_t *)(buffer + offsetof(dhcp_message_t, flags));
    *flags_ptr = htons(msg->flags);

    // Add end option marker to the options field
    buffer[offsetof(dhcp_message_t, options) + 3] = 255; // Option 255 marks the end

    return 0;
}

// Function to set the DHCP message type in the options field
int set_dhcp_message_type(dhcp_message_t *msg, uint8_t type)
{
    if (!msg)
        return -1;

    // Validate that the options field has enough space (assuming options size is limited)
    if (sizeof(msg->options) < 3)
        return -1;

    msg->options[0] = 53;   // Option 53 is DHCP message type
    msg->options[1] = 1;    // Length of the DHCP message type option
    msg->options[2] = type; // Set the actual DHCP message type (e.g., DHCP_DISCOVER, DHCP_OFFER, etc.)

    return 0; // Success
}


// Function to print the DHCP message in an organized way
void print_dhcp_message(const dhcp_message_t *msg) {
    printf(BOLD BLUE "\n==================== DHCP MESSAGE ====================\n" RESET);

    printf(BOLD CYAN "%-25s: " RESET GREEN "%d\n" RESET, "Operation Code (op)", msg->op);
    printf(BOLD CYAN "%-25s: " RESET GREEN "0x%08X\n" RESET, "Transaction ID (xid)", msg->xid);

    // Print client IP address (ciaddr)
    struct in_addr client_ip;
    client_ip.s_addr = msg->ciaddr;
    printf(BOLD CYAN "%-25s: " RESET GREEN "%s\n" RESET, "Client IP Address", inet_ntoa(client_ip));

    // Print offered IP address (yiaddr)
    struct in_addr your_ip;
    your_ip.s_addr = msg->yiaddr;
    printf(BOLD CYAN "%-25s: " RESET GREEN "%s\n" RESET, "Offered IP (Your IP)", inet_ntoa(your_ip));

    // Print server IP address (siaddr)
    struct in_addr server_ip;
    server_ip.s_addr = msg->siaddr;
    printf(BOLD CYAN "%-25s: " RESET GREEN "%s\n" RESET, "Server IP Address", inet_ntoa(server_ip));

    // Print gateway IP address (giaddr)
    struct in_addr gateway_ip;
    gateway_ip.s_addr = msg->giaddr;
    printf(BOLD CYAN "%-25s: " RESET GREEN "%s\n" RESET, "Gateway IP Address", inet_ntoa(gateway_ip));


    // Print client MAC address
    printf(BOLD CYAN "%-25s: " RESET, "Client MAC Address");
    for (int i = 0; i < msg->hlen; i++) {
        printf(MAGENTA "%02x" RESET, msg->chaddr[i]);
        if (i < msg->hlen - 1) {
            printf(":");
        }
    }
    printf("\n");

    // Search and print subnet mask (option 1)
    const uint8_t *options = msg->options;
    size_t options_length = sizeof(msg->options);
    size_t i = 0;
    int subnet_mask_found = 0;

    while (i < options_length) {
        uint8_t option = options[i++];
        if (option == 255) break; // End of options
        uint8_t length = options[i++];

        if (option == 1 && length == 4) {
            printf(BOLD CYAN "%-25s: " RESET GREEN "%d.%d.%d.%d\n" RESET, 
                   "Subnet Mask", options[i], options[i+1], options[i+2], options[i+3]);
            subnet_mask_found = 1;
        }
        i += length;
    }

    if (!subnet_mask_found) {
        printf(BOLD CYAN "%-25s: " RESET RED "Not specified\n" RESET, "Subnet Mask");
    }

    // Print other DHCP options
    printf(BOLD YELLOW "\n==================== DHCP Options ====================\n" RESET);
    printf("\n");
    i = 0;
    while (i < options_length) {
        uint8_t option = options[i++];
        if (option == 255) break;
        uint8_t length = options[i++];

        switch (option) {
            case 51: // Lease Time
                printf(YELLOW "%-25s: " RESET GREEN "%d seconds\n" RESET, 
                       "IP Address Lease Time", ntohl(*(uint32_t *)&options[i]));
                break;

            case 53: // DHCP Message Type (Already printed, but left here for consistency)
                printf(YELLOW "%-25s: " RESET RED "%d (%s)\n" RESET, 
                       "DHCP Message Type", options[i], get_dhcp_message_type_name(options[i]));
                break;

            case 54: // DHCP Server Identifier
                printf(YELLOW "%-25s: " RESET GREEN "%d.%d.%d.%d\n" RESET, 
                       "DHCP Server Identifier", options[i], options[i+1], options[i+2], options[i+3]);
                break;

            default: break;
        }

        i += length;
    }

    printf(BOLD YELLOW "\n======================================================\n" RESET);
}
