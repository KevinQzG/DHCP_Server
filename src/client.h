#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>
#include <stdint.h>
#include "./data/message.h" 


// Macro definitions
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define MAGENTA "\033[35m"
#define RED "\033[31m"

#define MAX_CHARACTERS 360
#define BUFFER_SIZE 1024 // Buffer size for incoming messages, maximum size of a DHCP message is 1024 bytes
#define SOCKET_ADDRESS struct sockaddr // Define SOCKET_ADDRESS as struct sockaddr 

// Global variable for socket descriptor
extern int sockfd;

// Function declarations
int get_mac_address(uint8_t *mac, const char *iface);
void handle_signal_interrupt(int signal);
void end_program();
void handle_dhcp_offer(int sockfd, struct sockaddr_in *server_addr, dhcp_message_t *offer_msg);
int recv_dhcp_ack(int sockfd, struct sockaddr_in *server_addr);

#endif // CLIENT_H
