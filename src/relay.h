#ifndef RELAY_H
#define RELAY_H

#include <netinet/in.h>
#include <stdint.h>
#include "./config/env.h" 

#define MAX_CHARACTERS 360
#define BUFFER_SIZE 1024 // Buffer size for incoming messages, maximum size of a DHCP message is 1024 bytes
#define SOCKET_ADDRESS struct sockaddr // Define SOCKET_ADDRESS as struct sockaddr 

// Function declaration
void end_program();
void handle_signal_interrupt(int signal);
void *dhcp_client_listener(void *arg);
void *dhcp_server_listener(void *arg);

#endif
