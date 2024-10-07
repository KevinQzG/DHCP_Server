#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#define DHCP_OPTIONS_LENGTH 312 // Maximum length of DHCP options field
#define HARDWARE_ADDR_LEN 16 // Maximum length of hardware address (MAC address)

#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_DECLINE 4
#define DHCP_ACK 5
#define DHCP_NAK 6
#define DHCP_RELEASE 7

#define DHCP_MAGIC_COOKIE 0x63825363

// DHCP message structure
typedef struct {
    uint8_t op;                   // Message op code / message type. 1 = BOOTREQUEST, 2 = BOOTREPLY
    uint8_t htype;                // Hardware address type (e.g., '1' = Ethernet)
    uint8_t hlen;                 // Hardware address length (e.g., '6' for MAC addresses)
    uint8_t hops;                 // Hops

    uint32_t xid;                 // Transaction ID (random number to identify the request/response)
    uint16_t secs;                // Seconds elapsed since client started trying to boot
    uint16_t flags;               // Flags (e.g., broadcast flag)

    uint32_t ciaddr;              // Client IP address (if already has one)
    uint32_t yiaddr;              // 'Your' (client) IP address assigned by the server
    uint32_t siaddr;              // Next server IP address to use in the bootstrap process
    uint32_t giaddr;              // Gateway IP address

    uint8_t chaddr[HARDWARE_ADDR_LEN];  // Client hardware address (MAC address)
    uint8_t sname[64];            // Optional server host name
    uint8_t file[128];            // Boot file name

    uint8_t options[DHCP_OPTIONS_LENGTH]; // Optional parameters field (e.g., message type, lease time)
} dhcp_message_t;

// Function to initialize a DHCP message structure
void init_dhcp_message(dhcp_message_t *msg);

// Function to parse raw data into a dhcp_message_t structure
int parse_dhcp_message(const uint8_t *buffer, dhcp_message_t *msg);

// Function to serialize a dhcp_message_t structure into a raw byte buffer
int build_dhcp_message(const dhcp_message_t *msg, uint8_t *buffer);

// Function to set the DHCP message type in the options field
void set_dhcp_message_type(dhcp_message_t *msg, uint8_t type);

// Function to print the contents of a DHCP message
void print_dhcp_message(const dhcp_message_t *msg);


#endif