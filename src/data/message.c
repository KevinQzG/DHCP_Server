#include "./message.h"

#include <stddef.h>   // For offsetof
#include <string.h> // For memset, memcpy
#include <stdio.h>  // For debugging purposes

#include <arpa/inet.h> // For htonl, ntohl, htons, ntohs

// Function to initialize a DHCP message structure with default values
void init_dhcp_message(dhcp_message_t *msg) {
    memset(msg, 0, sizeof(dhcp_message_t)); // Clear all fields to 0

    msg->op = 1;    // BOOTREQUEST (client to server)
    msg->htype = 1; // Ethernet
    msg->hlen = 6;  // MAC address length
    msg->hops = 0;  // Hops (usually 0 for clients)
    msg->xid = 0;   // Set a unique transaction ID later
    msg->flags = 0; // Set broadcast or other flags if needed
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

// Function to serialize a dhcp_message_t structure into a raw byte buffer
int build_dhcp_message(const dhcp_message_t *msg, uint8_t *buffer) {
    // Clear buffer
    memset(buffer, 0, sizeof(dhcp_message_t));

    // Copy the DHCP message into the buffer
    memcpy(buffer, msg, sizeof(dhcp_message_t));

    // Perform necessary byte-order conversions before sending (for multi-byte fields)
    uint32_t *xid_ptr = (uint32_t*)(buffer + offsetof(dhcp_message_t, xid));
    *xid_ptr = htonl(msg->xid);  // Convert host byte order to network byte order

    uint16_t *secs_ptr = (uint16_t*)(buffer + offsetof(dhcp_message_t, secs));
    *secs_ptr = htons(msg->secs);

    uint16_t *flags_ptr = (uint16_t*)(buffer + offsetof(dhcp_message_t, flags));
    *flags_ptr = htons(msg->flags);

    // Perform similar conversions for ciaddr, yiaddr, siaddr, giaddr, etc.
    return 0; // Success
}

// Function to set the DHCP message type in the options field
void set_dhcp_message_type(dhcp_message_t *msg, uint8_t type) {
    if (!msg) return;

    msg->options[0] = 53;   // Option 53 is DHCP message type
    msg->options[1] = 1;    // Length of the DHCP message type option
    msg->options[2] = type; // Set the actual DHCP message type (e.g., DHCP_DISCOVER, DHCP_OFFER, etc.)
}

// Example function to print the DHCP message for debugging
void print_dhcp_message(const dhcp_message_t *msg) {
    printf("Message op: %d\n", msg->op);
    printf("Transaction ID: %u\n", msg->xid);
    printf("Client IP: %u\n", ntohl(msg->ciaddr));
    printf("Your IP: %u\n", ntohl(msg->yiaddr));
    printf("Server IP: %u\n", ntohl(msg->siaddr));
    printf("Gateway IP: %u\n", ntohl(msg->giaddr));
    printf("DHCP Message Type: %d\n", msg->options[2]);  // Assuming the message type is set
}
