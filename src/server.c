// Essential includes for C programs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Includes for socket creation
#include <sys/socket.h>  // For socket creation
#include <arpa/inet.h>   // For htons() function
#include <unistd.h>      // For close() function

// Include for Signal Handling
#include <signal.h>

// Includes for threads
#include <pthread.h>

// Personal includes
#include "./server.h"
#include "./config/env.h"
#include "./data/message.h"
#include "./config/db.h"
#include <time.h>        // For srand() and rand() functions
#include "data/ip_pool.h"

// Global variables
int sockfd;

// Function declarations
void send_dhcpoffer(int socket_fd, struct sockaddr_in* client_addr, dhcp_message_t* discover_message);
void end_program();
void handle_signal_interrupt(int signal);
void* proccess_client_connection(void *arg);
void generate_dynamic_gateway_ip(char* gateway_ip, size_t size);

int main(int argc, char *argv[]) {
    srand(time(NULL));

    // Server and client structures
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_addr_len = sizeof(client_addr);

    // Load environment variables and initialize database
    load_env_variables();
    init_db();

    // Register signal handler for SIGINT (CTRL+C)
    signal(SIGINT, handle_signal_interrupt);

    // Initialize IP pool
    init_ip_pool();

    // Create the socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // AF_INET: IPv4, SOCK_DGRAM: UDP
    if (sockfd < 0) {
        printf("Socket creation failed.\n");
        exit(0);
    }
    printf("Socket created successfully.\n");

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));       // Zero out the structure
    server_addr.sin_family = AF_INET;                   // IPv4
    server_addr.sin_addr.s_addr = inet_addr(server_ip);  // Server IP
    server_addr.sin_port = htons(port);                 // Convert port to network byte order

    // Bind the socket
    if (bind(sockfd, (SOCKET_ADDRESS *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Socket bind failed.\n");
        close(sockfd);
        exit(0);
    }
    printf("Socket bind successful.\n");
    printf("UDP server is running on %s:%d...\n", server_ip, port);

    // Infinite loop to keep the server running
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        // Receive data from client
        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (SOCKET_ADDRESS *)&client_addr, &client_addr_len);
        if (recv_len < 0) {
            printf("Failed to receive data.\n");
            continue;
        }

        // Allocate memory for client data
        client_data_t *client_data = (client_data_t *)malloc(sizeof(client_data_t));
        if (!client_data) {
            printf("Failed to allocate memory for client data.\n");
            continue;
        }

        // Populate client data
        client_data->sockfd = sockfd;
        memcpy(client_data->buffer, buffer, BUFFER_SIZE);
        client_data->client_addr = client_addr;
        client_data->client_addr_len = client_addr_len;

        // Create thread to handle client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, proccess_client_connection, (void *)client_data) != 0) {
            printf("Failed to create thread.\n");
            free(client_data);
            continue;
        }

        // Detach thread
        pthread_detach(thread_id);
    }

    // Clean up and exit
    end_program();
    return 0;
}

// Function to handle client connection
void *proccess_client_connection(void *arg) {
    client_data_t *data = (client_data_t *)arg;
    char *buffer = data->buffer;
    struct sockaddr_in client_addr = data->client_addr;
    socklen_t client_addr_len = data->client_addr_len;
    int connection_sockfd = data->sockfd;

    printf("Processing DHCP message from client %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    dhcp_message_t dhcp_msg;

    // Parse DHCP message
    if (parse_dhcp_message((uint8_t *)buffer, &dhcp_msg) != 0) {
        printf("Failed to parse DHCP message.\n");
        free(data);
        return NULL;
    }

    print_dhcp_message(&dhcp_msg);

    uint8_t dhcp_message_type = 0;

    // Extract DHCP message type
    for (int i = 0; i < DHCP_OPTIONS_LENGTH; i++) {
        if (dhcp_msg.options[i] == 53) {
            dhcp_message_type = dhcp_msg.options[i + 1];
            break;
        }
    }

    // Handle DHCP_DISCOVER message
    if (dhcp_message_type == DHCP_DISCOVER) {
        send_dhcpoffer(connection_sockfd, &client_addr, &dhcp_msg);
    }

    const char *response = "DHCP server response";
    sendto(connection_sockfd, response, strlen(response), 0, (SOCKET_ADDRESS *)&client_addr, client_addr_len);

    free(data);
    return NULL;
}

// Function to handle signal interrupt
void handle_signal_interrupt(int signal) {
    printf("\nSignal %d received.\n", signal);
    end_program();
}

// Function to close socket and free resources
void end_program() {
    if (sockfd >= 0) close(sockfd);
    if (ip_pool) free(ip_pool);
    printf("Exiting...\n");
    exit(0);
}

// Function to send DHCP offer
void send_dhcpoffer(int socket_fd, struct sockaddr_in* client_addr, dhcp_message_t* discover_message) {
    dhcp_message_t offer_message;
    init_dhcp_message(&offer_message);

    if (discover_message->hlen == 6) {
        memcpy(offer_message.chaddr, discover_message->chaddr, 6);
    } else {
        printf("Error: Invalid MAC address length.\n");
        return;
    }

    char *assigned_ip = assign_ip();
    if (!assigned_ip) {
        printf("No IP available to offer.\n");
        return;
    }

    inet_pton(AF_INET, assigned_ip, &offer_message.yiaddr);
    inet_pton(AF_INET, server_ip, &offer_message.siaddr);
    inet_pton(AF_INET, "0.0.0.0", &offer_message.ciaddr);

    char dynamic_gateway_ip[16];
    generate_dynamic_gateway_ip(dynamic_gateway_ip, sizeof(dynamic_gateway_ip));
    inet_pton(AF_INET, dynamic_gateway_ip, &offer_message.giaddr);

    offer_message.options[0] = 53;
    offer_message.options[1] = 1;
    offer_message.options[2] = DHCP_OFFER;
    offer_message.options[3] = 1;
    offer_message.options[4] = 4;
    inet_pton(AF_INET, "255.255.255.0", &offer_message.options[5]);
    offer_message.options[9] = 255;

    print_dhcp_message(&offer_message);

    int bytes_sent = sendto(socket_fd, &offer_message, sizeof(offer_message), 0, (struct sockaddr *)client_addr, sizeof(*client_addr));
    if (bytes_sent == -1) {
        perror("Error sending DHCPOFFER");
    }
}

// Function to generate a dynamic gateway IP
void generate_dynamic_gateway_ip(char* gateway_ip, size_t size) {
    snprintf(gateway_ip, size, "192.168.1.1");
}
