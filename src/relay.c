// Personal includes
#include "./relay.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>

// Define the socket variable in a global scope so that it can be accessed by the signal handler
int client_sockfd, server_sockfd;
struct sockaddr_in server_addr, client_addr, client_subnet;


// Function to clean up and terminate the program
void end_program() {
    if (client_sockfd >= 0)
        close(client_sockfd);

    if (server_sockfd >= 0)
        close(server_sockfd);

    printf("Exiting...\n");
    exit(0);
}


void handle_signal_interrupt(int signal) {
    printf(YELLOW "\nSignal %d received.\n" RESET, signal);
    end_program();
}


// Function to listen for DHCP Offer/ACK in a separate thread
void *dhcp_client_listener(void *arg) {
    socklen_t addr_len = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE];

    while (1) {
        // Clean the buffer
        memset(buffer, 0, sizeof(buffer));
        // Listen for incoming DHCP OFFER or ACK
        ssize_t recv_len = recvfrom(client_sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (recv_len < 0) {
            printf(RED "Failed to receive data from client.\n" RESET);
            continue;
        }

        ssize_t sent_bytes = sendto(server_sockfd, buffer, recv_len, 0, (struct sockaddr*)&server_addr, addr_len);

        if (sent_bytes < 0) {
            printf(RED "Failed to forward message to server.\n" RESET);
            continue;
        }

        printf(CYAN "DHCP message forwarded to server.\n" RESET);
    }

    return NULL;
}

void *dhcp_server_listener(void *arg) {
    socklen_t addr_len = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE];

    while (1) {
        // Clean the buffer
        memset(buffer, 0, sizeof(buffer));
        // Listen for incoming DHCP OFFER or ACK
        ssize_t recv_len = recvfrom(server_sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (recv_len < 0) {
            printf(RED "Failed to receive data from server.\n" RESET);
            continue;
        }

        ssize_t sent_bytes = sendto(client_sockfd, buffer, recv_len, 0, (struct sockaddr*)&client_addr, addr_len);

        if (sent_bytes < 0) {
            printf(RED "Failed to forward message to client.\n" RESET);
            continue;
        }

        printf(CYAN "DHCP message forwarded to client.\n" RESET);
    }

    return NULL;
}


int main() {
    // uint8_t buffer[sizeof(dhcp_message_t)];
    char buffer[BUFFER_SIZE];

    // Load environment variables
    load_env_variables();

    // Register the signal handler for SIGINT (CTRL+C)
    signal(SIGINT, handle_signal_interrupt);

    // Initialize the client socket
    client_sockfd = socket(AF_INET, SOCK_DGRAM, 0); // AF_INET: IPv4, SOCK_DGRAM: UDP

    if (client_sockfd < 0) {
        printf(RED "Client Socket creation failed.\n" RESET);
        exit(0);
    } else {
        printf(GREEN "Client Socket created successfully.\n" RESET);
    }

    // Initialize the server socket
    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0); // AF_INET: IPv4, SOCK_DGRAM: UDP

    if (server_sockfd < 0) {
        printf(RED "Server Socket creation failed.\n" RESET);
        exit(0);
    } else {
        printf(GREEN "Server Socket created successfully.\n" RESET);
    }

    int enable_broadcast = 1;
    if (setsockopt(client_sockfd, SOL_SOCKET, SO_BROADCAST, &enable_broadcast, sizeof(enable_broadcast)) < 0) {
        perror("Error setting SO_BROADCAST");
        close(client_sockfd);
        close(server_sockfd);
        return -1;
    }

    if (setsockopt(server_sockfd, SOL_SOCKET, SO_BROADCAST, &enable_broadcast, sizeof(enable_broadcast)) < 0) {
        perror("Error setting SO_BROADCAST");
        close(client_sockfd);
        close(server_sockfd);
        return -1;
    }

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Listen on all available interfaces

    if (bind(client_sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        printf(RED "Client Socket bind failed.\n" RESET);
        close(client_sockfd);
        close(server_sockfd);
        exit(0);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Create a thread to listen to incoming messages from the client
    pthread_t client_listener_thread;
    if (pthread_create(&client_listener_thread, NULL, dhcp_client_listener, NULL) != 0) {
        printf(RED "Failed to create client listener thread.\n" RESET);
        end_program();
    }

    // Create a thread to listen to incoming messages from the server
    pthread_t server_listener_thread;
    if (pthread_create(&server_listener_thread, NULL, dhcp_server_listener, NULL) != 0) {
        printf(RED "Failed to create server listener thread.\n" RESET);
        end_program();
    }

    while (1) {
        sleep(1);
    }
}
