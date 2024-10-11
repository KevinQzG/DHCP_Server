#include "ip_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../config/env.h"

// Define the IP pool as a pointer so it can be dynamic
ip_pool_entry_t* ip_pool = NULL;
int pool_size = 0;
char gateway_ip[16];  // Gateway IP address (it will be the first IP in the range)

// Function to calculate the size of the IP pool based on the dynamic range
int calculate_pool_size(char* start_ip, char* end_ip) {
    unsigned int start = ip_to_int(start_ip);
    unsigned int end = ip_to_int(end_ip);
    return (end - start + 1);  // The pool size is the difference between the start and end IP
}

// Function to convert an IP in text format to an integer
unsigned int ip_to_int(const char* ip) {
    unsigned int a, b, c, d;
    sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d);
    return (a << 24) | (b << 16) | (c << 8) | d;
}

// Function to convert an integer to an IP in text format
void int_to_ip(unsigned int ip, char* buffer) {
    sprintf(buffer, "%u.%u.%u.%u", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

// Function to initialize the IP pool based on the range
void init_ip_pool() {
    // Use the ip_range variable loaded from the .env file
    if (strlen(ip_range) == 0) {
        printf("No IP range specified in .env\n");
        return;
    }

    // Split the start and end IP
    char start_ip[16], end_ip[16];
    sscanf(ip_range, "%[^-]-%s", start_ip, end_ip);

    // Calculate the pool size
    pool_size = calculate_pool_size(start_ip, end_ip);

    // Allocate memory for the IP pool
    ip_pool = (ip_pool_entry_t*)malloc(pool_size * sizeof(ip_pool_entry_t));
    if (ip_pool == NULL) {
        printf("Failed to allocate memory for IP pool.\n");
        return;
    }

    // Assign the first IP in the range as the gateway
    strncpy(gateway_ip, start_ip, sizeof(gateway_ip));

    // Convert the start and end IPs to integers
    unsigned int start = ip_to_int(start_ip);
    unsigned int end = ip_to_int(end_ip);

    int i = 0;
    char ip_buffer[16];

    // Generate all the IPs within the range and store them in the pool
    for (unsigned int ip = start; ip <= end && i < pool_size; ip++) {
        int_to_ip(ip, ip_buffer);
        strncpy(ip_pool[i].ip_address, ip_buffer, sizeof(ip_pool[i].ip_address));
        ip_pool[i].is_assigned = 0;  // Mark as unassigned
        i++;
    }

    printf("Gateway IP set to: %s\n", gateway_ip);  // Display the gateway IP
}

char* get_gateway_ip() {
    return gateway_ip;  // Return the gateway address
}

char *assign_ip()
{
    for (int i = 0; i < pool_size; i++)
    {
        if (!ip_pool[i].is_assigned)
        {
            ip_pool[i].is_assigned = 1; // Mark IP as assigned
            return ip_pool[i].ip_address; // Return the available IP
        }
    }
    return NULL; // Return NULL if no available IPs
}
