#include "./message.h"
#include "../config/env.h"

#include <stddef.h> // For offsetof
#include <string.h> // For memset, memcpy
#include <stdio.h>  // For debugging purposes
#include <time.h>   // For time()
#include <stdlib.h> // For srand and rand

#include <arpa/inet.h> // For htonl, ntohl, htons, ntohs

// Definiciones de colores ANSI
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define MAGENTA "\033[35m"
#define RED "\033[31m"


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
        if (option == 255)
            break; // End of options

        uint8_t length = options[i++];
        if (i + length > options_length)
        {
            printf("Malformed option, exceeding buffer length\n");
            break;
        }

        switch (option)
        {
        case 1: // Subnet Mask
            printf("Subnet Mask: %d.%d.%d.%d\n", options[i], options[i + 1], options[i + 2], options[i + 3]);
            break;

        case 51: // Lease Time
        {
            uint32_t lease_time = ntohl(*(uint32_t *)&options[i]); // Convertir el tiempo de arrendamiento a host byte order
            printf("IP Address Lease Time: %d seconds (%d hours, %d minutes)\n",
                   lease_time,
                   lease_time / 3600,
                   (lease_time % 3600) / 60);
        }
        break;
        case 53: // DHCP Message Type
            printf("DHCP Message Type: %d\n", options[i]);
            break;
        case 54: // DHCP Server Identifier
            printf("DHCP Server Identifier: %d.%d.%d.%d\n", options[i], options[i + 1], options[i + 2], options[i + 3]);
            break;
        case 6: // DNS Server
            printf("DNS Server: %d.%d.%d.%d\n", options[i], options[i + 1], options[i + 2], options[i + 3]);
            break;
        // Add more case statements for other options as needed
        default:
            printf("Unhandled Option: %d\n", option);
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

void print_dhcp_message(const dhcp_message_t *msg)
{
    printf(BOLD BLUE "\n==================== DHCP MESSAGE ====================\n" RESET);

    printf(BOLD CYAN "Operation Code (op)     " RESET ": " GREEN "%d\n" RESET, msg->op);
    printf(BOLD CYAN "Transaction ID (xid)    " RESET ": " GREEN "0x%08X\n" RESET, msg->xid);

    // Imprimir Client IP (ciaddr)
    struct in_addr client_ip;
    client_ip.s_addr = msg->ciaddr;
    printf(BOLD CYAN "Client IP Address       " RESET ": " GREEN "%s\n" RESET, inet_ntoa(client_ip));

    // Imprimir Your IP (yiaddr) - la IP ofrecida por el servidor
    struct in_addr your_ip;
    your_ip.s_addr = msg->yiaddr;
    printf(BOLD CYAN "Offered IP (Your IP)    " RESET ": " GREEN "%s\n" RESET, inet_ntoa(your_ip));

    // Imprimir Server IP (siaddr)
    struct in_addr server_ip;
    server_ip.s_addr = msg->siaddr;
    printf(BOLD CYAN "Server IP Address       " RESET ": " GREEN "%s\n" RESET, inet_ntoa(server_ip));

    // Imprimir Gateway IP (giaddr)
    struct in_addr gateway_ip;
    gateway_ip.s_addr = msg->giaddr;
    printf(BOLD CYAN "Gateway IP Address      " RESET ": " GREEN "%s\n" RESET, inet_ntoa(gateway_ip));

    // Imprimir DNS Server IP Address solo si es un OFFER o ACK
    if (msg->options[2] == DHCP_OFFER || msg->options[2] == DHCP_ACK) {
        // Imprimir DNS Server IP Address
        printf(BOLD CYAN "DNS Server IP Address   " RESET ": " GREEN "%s\n" RESET, global_dns_ip);
    } else {
        printf(BOLD CYAN "DNS Server IP Address   " RESET ": " RED "Not assigned\n" RESET);
    }

    // Imprimir Client MAC Address
    printf(BOLD CYAN "Client MAC Address      " RESET ": ");
    for (int i = 0; i < msg->hlen; i++) {
        printf(MAGENTA "%02x" RESET, msg->chaddr[i]);
        if (i < msg->hlen - 1)
            printf(":");
    }
    printf("\n");

    // Parsear las opciones para obtener la submáscara de red antes de imprimir las demás opciones
    const uint8_t *options = msg->options;
    size_t options_length = sizeof(msg->options);
    size_t i = 0;
    int subnet_mask_found = 0;

    // Buscar la submáscara de red (opción 1)
    while (i < options_length) {
        uint8_t option = options[i++];
        if (option == 255)
            break; // Fin de las opciones
        uint8_t length = options[i++];

        if (option == 1 && length == 4) { // Opción 1 es la Subnet Mask
            printf(BOLD CYAN "Subnet Mask             " RESET ": " GREEN "%d.%d.%d.%d\n" RESET,
                   options[i], options[i + 1], options[i + 2], options[i + 3]);
            subnet_mask_found = 1;
        }
        i += length; // Avanzar al siguiente
    }

    // Si no se encuentra la submáscara de red, imprimir un valor por defecto o un mensaje
    if (!subnet_mask_found)
    {
        printf(BOLD CYAN "Subnet Mask             " RESET ": " RED "Not specified\n" RESET);
    }

    printf(BOLD YELLOW "\n==================== DHCP TYPE ====================\n" RESET);
    printf("\n");

    // Imprimir el resto de las opciones, excluyendo la submáscara de red (opción 1)
    i = 0;
    while (i < options_length)
    {
        uint8_t option = options[i++];
        if (option == 255)
            break; // Fin de las opciones
        uint8_t length = options[i++];

        switch (option)
        {
        case 1: 
            break;

        case 51: // Lease Time
            printf(YELLOW "IP Address Lease Time" RESET ": " GREEN "%d\n" RESET, ntohl(*(uint32_t *)&options[i]));
            break;

        case 53: // DHCP Message Type
            printf(YELLOW "DHCP Message Type    " RESET ": " RED "%d (%s)\n" RESET, options[i], get_dhcp_message_type_name(options[i]));
            break;

        default:
            break;
        }

        i += length; // Avanzar al siguiente
    }

    printf(BOLD YELLOW "\n======================================================\n" RESET);
}
