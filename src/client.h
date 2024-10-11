#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>

#define MAX_CHARACTERS 360
#define BUFFER_SIZE 1024 // Buffer size for incoming messages, maximum size of a DHCP message is 1024 bytes
#define SOCKET_ADDRESS struct sockaddr // Define SOCKET_ADDRESS as struct sockaddr 

// Function to handle signal interrupt (Ctrl+C)
void handle_signal_interrupt(int signal);

// Function to end the program
void end_program();

// Function to handle the DHCP Offer message
void handle_dhcp_offer(int sockfd, struct sockaddr_in *server_addr, dhcp_message_t *offer_msg);

// Function to receive a DHCP ACK message from the server
int recv_dhcp_ack(int sockfd, struct sockaddr_in *server_addr);

#endif