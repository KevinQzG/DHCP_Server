#ifndef IP_POOL_H
#define IP_POOL_H

#include <stdint.h>  // To use uint32_t

extern int pool_size;  // Declaration of the pool size

int is_ip_available(uint32_t requested_ip);

// Structure to handle IP addresses
typedef struct {
    char ip_address[16];  // IP address in text format
    int is_assigned;      // IP status: 0 = free, 1 = assigned
} ip_pool_entry_t;

// Declaration of the IP pool (size not specified here, it will be dynamic)
extern ip_pool_entry_t* ip_pool;

// Functions to manage the IP pool
void init_ip_pool();  // Initializes the IP pool
char* assign_ip();    // Assigns an available IP from the pool
char* get_gateway_ip();  // New declaration

// Function declarations to convert IP to integer and vice versa
unsigned int ip_to_int(const char* ip);
void int_to_ip(unsigned int ip, char* buffer);

// Function to calculate the IP pool size based on the dynamic range
int calculate_pool_size(char* start_ip, char* end_ip);

#endif
