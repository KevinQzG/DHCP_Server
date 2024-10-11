#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

#include "./data/message.h"

#define MAX_CHARACTERS 360
#define BUFFER_SIZE 1024 // Buffer size for incoming messages, maximum size of a DHCP message is 1024 bytes
#define SOCKET_ADDRESS struct sockaddr // Define SOCKET_ADDRESS as struct sockaddr 
#define MAX_CLIENTS 100 // Define the maximum number of clients

// Function Declarations
void *proccess_client_connection(void *arg);
void handle_signal_interrupt(int signal);
void end_program();
void send_dhcpoffer(int socket_fd, struct sockaddr_in *client_addr, dhcp_message_t *discover_message);
void generate_dynamic_gateway_ip(char *gateway_ip, size_t size);
void handle_dhcp_request(int sockfd, struct sockaddr_in *client_addr, dhcp_message_t *request_msg);
void init_server_addr(struct sockaddr_in *server_addr);
void set_dhcp_message_options(dhcp_message_t *msg, int type);
void handle_dhcp_release(int sockfd, dhcp_message_t *release_msg);

// Structure to pass client information to threads
typedef struct {
    int sockfd; 
    struct sockaddr_in client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_addr_len;
} client_data_t;

// Structure to keep track of processed clients to avoid duplicates
typedef struct client_record
{
    uint8_t mac[6];
    time_t timestamp; // Timestamp for controlling duplicates in a short time
} client_record_t;

#endif