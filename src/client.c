// Essential includes for c programs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Includes for socket creation
#include <sys/socket.h>  // For socket creation
#include <arpa/inet.h> // For htons() function
#include <unistd.h> // For close() function

// Include for Signal Handling
#include <signal.h>

// Personal includes
#include "./client.h"
#include "./config/env.h"
#include "./data/message.h"

// Define the socket variable in a global scope so that it can be accessed by the signal handler
int sockfd;

void end_program() {
    // Close the socket just if it was created
    if (sockfd >= 0){
        close(sockfd);
    }
    printf("Exiting...\n");
    exit(0);
}

void handle_signal_interrupt(int signal) {
    printf("\n");
    printf("Signal %d received.\n", signal);

    // Call the function to close the socket and exit the program
    end_program();
}

int main() {
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Load environment variables
    load_env_variables();

    // Register the signal handler for SIGINT (CTRL+C)
    signal(SIGINT, handle_signal_interrupt);

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

    dhcp_message_t msg;
    uint8_t buffer[sizeof(dhcp_message_t)];

    // Initialize a DHCP Discover message
    init_dhcp_message(&msg);
    set_dhcp_message_type(&msg, DHCP_DISCOVER);

    // Serialize the message to a buffer
    build_dhcp_message(&msg, buffer);

    // Send message to server
    int sent_bytes = sendto(sockfd, buffer, sizeof(buffer), 0, (SOCKET_ADDRESS*)&server_addr, addr_len);
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

    // Call the function to close the socket and exit the program
    end_program();
    return 0;
}
