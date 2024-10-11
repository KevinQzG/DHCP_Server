#ifndef UTILS_H
#define UTILS_H

#include <netinet/in.h>

// Function to get the MAC address of a network interface
int get_mac_address(uint8_t *mac, const char *iface);

#endif