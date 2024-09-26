#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "172.26.70.2" // Replace with the server's actual IP address
#define SERVER_PORT 8080        // Replace with your server's port
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = "Hello from UDP client!";
    socklen_t addr_len = sizeof(server_addr);

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("Failed to create socket.\n");
        return -1;
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Send message to server
    int sent_bytes = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, addr_len);
    if (sent_bytes < 0) {
        printf("Failed to send message to server.\n");
        close(sockfd);
        return -1;
    }

    printf("Message sent to server: %s\n", buffer);

    // Receive server's response
    memset(buffer, 0, BUFFER_SIZE);
    int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &addr_len);
    if (recv_len > 0) {
        printf("Server response: %s\n", buffer);
    } else {
        printf("Failed to receive response from server.\n");
    }

    // Close the socket
    close(sockfd);
    return 0;
}
