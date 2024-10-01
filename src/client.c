#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

// Personal includes
#include "./client.h"
#include "./config/env.h"

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = "Hello from UDP client!";
    socklen_t addr_len = sizeof(server_addr);

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

    // Send message to server
    int sent_bytes = sendto(sockfd, buffer, strlen(buffer), 0, (SOCKET_ADDRESS*)&server_addr, addr_len);
    if (sent_bytes < 0) {
        printf("Failed to send message to server.\n");
        close(sockfd);
        return -1;
    }

    printf("Message sent to server: %s\n", buffer);

    // Receive server's response
    memset(buffer, 0, BUFFER_SIZE);
    int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (SOCKET_ADDRESS*)&server_addr, &addr_len);
    if (recv_len > 0) {
        printf("Server response: %s\n", buffer);
    } else {
        printf("Failed to receive response from server.\n");
    }

    // Close the socket
    close(sockfd);
    return 0;
}
