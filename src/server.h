#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

#include "./data/message.h"

#define MAX_CHARACTERS 360
#define BUFFER_SIZE 1024 // Buffer size for incoming messages, maximum size of a DHCP message is 1024 bytes
#define SOCKET_ADDRESS struct sockaddr // Define SOCKET_ADDRESS as struct sockaddr 
#define MAX_CLIENTS 100 // Define the maximum number of clients

// Structure to pass client information to threads
typedef struct {
    int sockfd;
    struct sockaddr_in client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_addr_len;
} client_data_t;


// Function Declarations
void end_program();
void handle_signal_interrupt(int signal) ;
void send_dhcp_offer(int socket_fd, struct sockaddr_in *client_addr, dhcp_message_t *discover_message);
void handle_dhcp_request(int sockfd, struct sockaddr_in *client_addr, dhcp_message_t *request_msg);
void handle_dhcp_release(int sockfd, dhcp_message_t *release_msg);
void *process_client_connection(void *arg);
void *check_and_release(void *arg);
void generate_dynamic_gateway_ip(char *gateway_ip, size_t size);

#endif