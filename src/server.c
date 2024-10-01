// Essential includes for c programs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Includes for socket creation
#include <sys/socket.h>  // For socket creation
#include <arpa/inet.h> // For htons() function
#include <unistd.h> // For close() function

// Includes for threads
#include <pthread.h>

// Personal includes
#include "./server.h"
#include "./config/env.h"

void* proccess_client_connection(void* arg){
    client_data_t* data = (client_data_t*)arg;
    char* buffer = data->buffer;
    struct sockaddr_in client_addr = data->client_addr;
    socklen_t client_addr_len = data->client_addr_len;
    int sockfd = data->sockfd;

    printf("Processing DHCP message from client %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Here you would process the DHCP message and respond appropriately.
    // You can parse the DHCP message here and decide whether to send a DHCPOFFER or another DHCP message.

    // Example: Send a basic response (this is just a placeholder, actual DHCP response would be more complex)
    const char *response = "DHCP server response";
    sendto(sockfd, response, strlen(response), 0, (SOCKET_ADDRESS*) &client_addr, client_addr_len);

    // Free the client_data_t structure that was dynamically allocated
    free(data);
    
    return NULL; 
}

int main(int argc, char *argv[]) {
    // Define a socket, port
    int sockfd;
    // Define a structure to hold the server address information
    struct sockaddr_in server_addr, client_addr;    
    // String buffer to hold incoming messages
    char buffer[BUFFER_SIZE];
    // Define a structure to hold the client address information
    socklen_t client_addr_len = sizeof(client_addr);

    // Load environment variables
    load_env_variables();

    // Initialize the created socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // AF_INET: IPv4, SOCK_DGRAM: UDP

    // Check if the socket was created successfully
    if (sockfd < 0) {
        printf("Socket creation failed.\n");
        exit(0);
    } else {
        printf("Socket created successfully.\n");
    }

    // Set the bytes in memory for the server_addr structure to 0
    memset(&server_addr, 0, sizeof(server_addr));  // Zero out the structure
    server_addr.sin_family = AF_INET;  // IPv4
    server_addr.sin_addr.s_addr = inet_addr(server_ip);  // Accept connections from the specified server IP not other IPS or interfaces
    server_addr.sin_port = htons(port);  // Convert port number to network byte order (port 67 for DHCP server)

    // Bind the socket to the port
    if (bind(sockfd, (SOCKET_ADDRESS*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Socket Bind failed.\n");
        close(sockfd);
        exit(0);
    } else {
        printf("Socket bind successful.\n");
    }

    printf("UDP server is running on %s:%d...\n", server_ip, port);

    // Infinite loop to keep the server running, opens threads to handle each incoming client
    while (1) {
        // Clear the buffer before receiving a new message
        memset(buffer, 0, BUFFER_SIZE);

        // Receive a message from the client
        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (SOCKET_ADDRESS*) &client_addr, &client_addr_len);
        if (recv_len < 0) {
            printf("Failed to receive data.\n");
            continue;
        }

        printf("Received DHCP message from client %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Dynamically allocate memory for client data
        client_data_t* client_data = (client_data_t*)malloc(sizeof(client_data_t));
        if (client_data == NULL) {
            printf("Failed to allocate memory for client data.\n");
            continue;
        }

        // Populate the client_data structure
        client_data->sockfd = sockfd;
        memcpy(client_data->buffer, buffer, BUFFER_SIZE);
        client_data->client_addr = client_addr;
        client_data->client_addr_len = client_addr_len;

        // Create a thread to handle the client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, proccess_client_connection, (void*)client_data) != 0) {
            printf("Failed to create thread.\n");
            free(client_data);
            continue;
        }

        // Detach the thread so that its resources are automatically reclaimed when it finishes
        pthread_detach(thread_id);
    }

    close(sockfd);
    return 0;
}
