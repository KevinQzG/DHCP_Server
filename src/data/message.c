#include "./message.h"

#include <stddef.h>   // For offsetof
#include <string.h> // For memset, memcpy
#include <stdio.h>  // For debugging purposes
#include <time.h> // For time()
#include <stdlib.h>  // For srand and rand

#include <arpa/inet.h> // For htonl, ntohl, htons, ntohs


// Function to initialize a DHCP message structure with default values
void init_dhcp_message(dhcp_message_t *msg) {
    memset(msg, 0, sizeof(dhcp_message_t)); // Clear all fields

    msg->op = 1;    // BOOTREQUEST (client to server)
    msg->htype = 1; // Ethernet
    msg->hlen = 6;  // MAC address length
    msg->hops = 0;  // Hops (usually 0 for clients)

    // Seed the random number generator with the current time (only done once)
    srand((unsigned int)time(NULL)); 

    // Use a random transaction ID
    msg->xid = rand(); 
    msg->secs = 0;   // No seconds elapsed
    msg->flags = htons(0x8000); // Broadcast flag set
}

// Function to parse raw data into a dhcp_message_t structure
int parse_dhcp_message(const uint8_t *buffer, dhcp_message_t *msg) {
    if (!buffer || !msg) return -1; // Sanity check for null pointers

    // Copy fields from the raw buffer to the structure
    memcpy(msg, buffer, sizeof(dhcp_message_t));

    // Perform any necessary byte-order conversions here (e.g., for multi-byte fields)
    msg->xid = ntohl(msg->xid);       // Convert network byte order to host byte order
    msg->secs = ntohs(msg->secs);
    msg->flags = ntohs(msg->flags);
    msg->ciaddr = ntohl(msg->ciaddr);
    msg->yiaddr = ntohl(msg->yiaddr);
    msg->siaddr = ntohl(msg->siaddr);
    msg->giaddr = ntohl(msg->giaddr);

    return 0; // Success
}

void parse_dhcp_options(const uint8_t *options, size_t options_length) {
    size_t i = 0;
    while (i < options_length) {
        uint8_t option = options[i++];
        if (option == 255) break; // End of options

        uint8_t length = options[i++];
        if (i + length > options_length) {
            printf("Malformed option, exceeding buffer length\n");
            break;
        }

        switch (option) {
            case 1:
                printf("Subnet Mask: %d.%d.%d.%d\n", options[i], options[i+1], options[i+2], options[i+3]);
                break;
            case 51:
                printf("IP Address Lease Time: %d\n", ntohl(*(uint32_t *)&options[i]));
                break;
            case 53: // DHCP Message Type
                printf("DHCP Message Type: %d\n", options[i]);
                break;
            case 54:
                printf("DHCP Server Identifier: %d.%d.%d.%d\n", options[i], options[i+1], options[i+2], options[i+3]);
                break;
            // Add more case statements for other options (like requested IP, lease time, etc.)
            default:
                printf("Unhandled Option: %d\n", option);
                break;
        }
        
        i += length; // Skip to the next option
    }
}


// Function to serialize a dhcp_message_t structure into a raw byte buffer
int build_dhcp_message(const dhcp_message_t *msg, uint8_t *buffer, size_t buffer_size) {
    if (buffer_size < sizeof(dhcp_message_t)) return -1; // Ensure the buffer is large enough

    // Clear buffer and copy DHCP message
    memset(buffer, 0, buffer_size);
    memcpy(buffer, msg, sizeof(dhcp_message_t));

    // Perform necessary byte-order conversions
    uint32_t *xid_ptr = (uint32_t*)(buffer + offsetof(dhcp_message_t, xid));
    *xid_ptr = htonl(msg->xid);

    uint16_t *secs_ptr = (uint16_t*)(buffer + offsetof(dhcp_message_t, secs));
    *secs_ptr = htons(msg->secs);

    uint16_t *flags_ptr = (uint16_t*)(buffer + offsetof(dhcp_message_t, flags));
    *flags_ptr = htons(msg->flags);

    // Add end option marker to the options field
    buffer[offsetof(dhcp_message_t, options) + 3] = 255;  // Option 255 marks the end

    return 0;
}


// Function to set the DHCP message type in the options field
int set_dhcp_message_type(dhcp_message_t *msg, uint8_t type) {
    if (!msg) return -1;

    // Validate that the options field has enough space (assuming options size is limited)
    if (sizeof(msg->options) < 3) return -1;

    msg->options[0] = 53;   // Option 53 is DHCP message type
    msg->options[1] = 1;    // Length of the DHCP message type option
    msg->options[2] = type; // Set the actual DHCP message type (e.g., DHCP_DISCOVER, DHCP_OFFER, etc.)
    
    return 0; // Success
}


// Example function to print the DHCP message for debugging
void print_dhcp_message(const dhcp_message_t *msg) {
    printf("Message op: %d\n", msg->op);
    printf("Transaction ID: %u\n", msg->xid);
    printf("Client IP: %u\n", ntohl(msg->ciaddr));
    printf("Your IP: %u\n", ntohl(msg->yiaddr));
    printf("Server IP: %u\n", ntohl(msg->siaddr));
    printf("Gateway IP: %u\n", ntohl(msg->giaddr));

    printf("Client MAC Address: ");
    for (int i = 0; i < msg->hlen; i++) {
        printf("%02x", msg->chaddr[i]);
        if (i < msg->hlen - 1) printf(":");
    }
    printf("\n");

    // Parse and print the DHCP options
    parse_dhcp_options(msg->options, sizeof(msg->options));
}

