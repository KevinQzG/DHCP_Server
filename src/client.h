#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>
#include <stdint.h>
#include "./data/message.h"
#include "./config/env.h" 
#include "./data/ip_pool.h"

#define MAX_CHARACTERS 360
#define BUFFER_SIZE 1024 // Buffer size for incoming messages, maximum size of a DHCP message is 1024 bytes
#define SOCKET_ADDRESS struct sockaddr // Define SOCKET_ADDRESS as struct sockaddr 

// Global variable for socket descriptor
extern int sockfd;

// Function declarations
void send_dhcp_release(int sockfd, struct sockaddr_in *server_addr);
void end_program();
void handle_signal_interrupt(int signal);
void send_dhcp_request(int sockfd, struct sockaddr_in *server_addr, dhcp_message_t *msg);
void handle_dhcp_offer(int sockfd, struct sockaddr_in *server_addr, dhcp_message_t *msg);
void renew_lease(int sockfd, struct sockaddr_in *server_addr);
void *dhcp_listener(void *arg);

#endif
