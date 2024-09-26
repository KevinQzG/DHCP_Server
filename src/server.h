#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

#define MAX_CHARACTERS 360
#define BUFFER_SIZE 1024 // Buffer size for incoming messages, maximum size of a DHCP message is 1024 bytes
#define socket_address struct sockaddr_in

// Structure to pass client information to threads
typedef struct {
    int sockfd;
    struct sockaddr_in client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_addr_len;
} client_data_t;

#endif