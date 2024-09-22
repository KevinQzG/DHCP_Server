// Essential includes for c programs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Includes for socket creation
#include <sys/socket.h>  // For socket creation
#include <netinet/in.h>  // For sockaddr_in structure
#include <arpa/inet.h> // For htons() function
#include <unistd.h> // For close() function

// Personal includes
#include "./main.h"
#include "./config/env.h"


int main(int argc, char *argv[]) {
    // Define a socket, port
    int sockfd;
    // Define a structure to hold the server address information
    struct sockaddr_in server_addr;

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
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Socket Bind failed.\n");
        close(sockfd);
        exit(0);
    } else {
        printf("Socket bind successful.\n");
    }

    printf("UDP server is running on %s:%d...\n", server_ip, port);

    close(sockfd);
    return 0;
}
