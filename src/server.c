// Essential includes for C programs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // To generate random numbers using srand() and rand()

// Includes for socket creation
#include <sys/socket.h> // For socket creation
#include <arpa/inet.h>  // For htons() function
#include <unistd.h>     // For close() function

// Include for Signal Handling
#include <signal.h>

// Includes for threads
#include <pthread.h>

// Personal includes
#include "./server.h"
#include "./config/env.h"
#include "data/ip_pool.h"

// Global variables
int sockfd;
char global_gateway_ip[16]; // Global variable for the gateway IP


// Function to clean up and terminate the program
void end_program() {
    if (sockfd >= 0)
        close(sockfd);
        
    if (ip_pool)
        free(ip_pool);

    printf("Exiting...\n");
    exit(0);
}


void handle_signal_interrupt(int signal) {
    printf("\nSignal %d received.\n", signal);
    end_program();
}


void send_dhcp_offer(int socket_fd, struct sockaddr_in *client_addr, dhcp_message_t *discover_message) {
    dhcp_message_t offer_message;
    init_dhcp_message(&offer_message);

    // Copy the client's MAC
    memcpy(offer_message.chaddr, discover_message->chaddr, 6);

    // Try to assign an IP from the pool
    char *assigned_ip = assign_ip();
    if (assigned_ip == NULL) {
        printf(RED "No available IP addresses in the pool.\n" RESET);

        // Set message type as DHCP_NAK
        set_dhcp_message_type(&offer_message, DHCP_NAK);
    } else {
        // Set the your IP address
        inet_pton(AF_INET, assigned_ip, &offer_message.yiaddr);

        // Set the server IP
        inet_pton(AF_INET, server_ip, &offer_message.siaddr);

        // Set the gateway IP
        inet_pton(AF_INET, global_gateway_ip, &offer_message.giaddr);

        // Set DHCP message type to DHCP_OFFER
        set_dhcp_message_type(&offer_message, DHCP_OFFER);

        // Add subnet mask (option 1)
        offer_message.options[3] = 1;
        offer_message.options[4] = 4;
        inet_pton(AF_INET, global_subnet_mask, &offer_message.options[5]);

        // Add DNS (option 6)
        offer_message.options[9] = 6;
        offer_message.options[10] = 4;
        inet_pton(AF_INET, global_dns_ip, &offer_message.options[11]);

        // Add lease time (option 51)
        offer_message.options[15] = 51;
        offer_message.options[16] = 4;
        uint32_t lease_time = htonl(LEASE_TIME);
        memcpy(&offer_message.options[17], &lease_time, 4);

        // End of options
        offer_message.options[21] = 255;
    }

    // Send DHCP_OFFER or DHCP_NAK message
    if (sendto(sockfd, &offer_message, sizeof(offer_message), 0, (struct sockaddr *) client_addr, sizeof(*client_addr)) < 0) {
        perror(RED "Error sending DHCP message" RESET);
    } else if (offer_message.options[2] == DHCP_OFFER) {
        printf(GREEN "DHCP_OFFER sent to client.\n" RESET);
    } else if (offer_message.options[2] == DHCP_NAK) {
        printf(RED "DHCP_NAK sent to client: IP not available.\n" RESET);
    }
}


void handle_dhcp_request(int sockfd, struct sockaddr_in *client_addr, dhcp_message_t *request_msg) {
    // Check if the client is requesting an IP that is no longer available or if there is an error in the request
    if (!is_ip_available(request_msg -> yiaddr)) {
        // Send a DHCP_NAK if the requested IP is unavailable
        printf(RED "Requested IP is not available, sending DHCP_NAK...\n" RESET);
        set_dhcp_message_type(request_msg, DHCP_NAK); // Set message type to DHCP_NAK
    } else {
        renew_lease(inet_ntoa(*(struct in_addr *)&request_msg -> yiaddr));
        printf(GREEN "Sending DHCP_ACK...\n" RESET);
        set_dhcp_message_type(request_msg, DHCP_ACK); // Set message type to DHCP_ACK
    }


    if (sendto(sockfd, request_msg, sizeof(*request_msg), 0, (struct sockaddr *)client_addr, sizeof(*client_addr)) < 0) {
        perror(RED "Error sending DHCP message" RESET);
    } else if (request_msg -> options[2] == DHCP_ACK) {
        printf(GREEN "DHCP_ACK sent to client.\n" RESET);
    } else if (request_msg -> options[2] == DHCP_NAK) {
        printf(RED "DHCP_NAK sent to client: IP not available.\n" RESET);
    }
}


void handle_dhcp_release(int sockfd, dhcp_message_t *release_msg) {
    // Free the IP address
    release_ip(inet_ntoa(*(struct in_addr *)&release_msg->ciaddr));

    // Print the DHCP_RELEASE message
    printf("IP address %s released.\n", inet_ntoa(*(struct in_addr *)&release_msg->ciaddr));
}


void *process_client_connection(void *arg) {
    client_data_t *data = (client_data_t *)arg;
    char *buffer = data->buffer;
    struct sockaddr_in client_addr = data->client_addr;
    socklen_t client_addr_len = data->client_addr_len;
    int connection_sockfd = data->sockfd;

    printf(CYAN "Processing DHCP message from client %s:%d\n" RESET, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    dhcp_message_t dhcp_msg;

    if (parse_dhcp_message((uint8_t *)buffer, &dhcp_msg) != 0) {
        printf(RED "Failed to parse DHCP message.\n" RESET);
        free(data);
        return NULL;
    }

    // Print the DHCP message with detailed formatting
    print_dhcp_message(&dhcp_msg, false);

    uint8_t dhcp_message_type = 0;
    for (int i = 0; i < DHCP_OPTIONS_LENGTH; i++) {
        if (dhcp_msg.options[i] == 53) {
            dhcp_message_type = dhcp_msg.options[i + 2]; // Corregido: obtener el valor real del tipo de mensaje
            break;
        }
    }

    switch (dhcp_message_type) {
    case DHCP_DISCOVER:
        printf(GREEN "Received DHCP_DISCOVER from client.\n" RESET);
        send_dhcp_offer(connection_sockfd, &client_addr, &dhcp_msg);
        break;

    case DHCP_REQUEST:
        printf(GREEN "Received DHCP_REQUEST from client.\n" RESET);
        handle_dhcp_request(connection_sockfd, &client_addr, &dhcp_msg);
        break;

    case DHCP_RELEASE:
        printf(GREEN "Received DHCP_RELEASE from client.\n" RESET);
        handle_dhcp_release(connection_sockfd, &dhcp_msg);
        break;

    default:
        printf(RED "Unrecognized DHCP message type: %d\n" RESET, dhcp_message_type);
        break;
    }

    free(data);
    return NULL;
}


void *check_and_release(void *arg) {
    while (1) {
        check_leases();
        sleep(1);
    }
}

// Function to generate a dynamic gateway IP based on the first IP of the range
void generate_dynamic_gateway_ip(char *gateway_ip, size_t size) {
    // Get the dynamic gateway IP from the IP pool
    strncpy(gateway_ip, get_gateway_ip(), size);
}


int main(int argc, char *argv[]) {
    srand(time(NULL));
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_addr_len = sizeof(client_addr);

    load_env_variables();

    printf(GREEN "Subnet loaded: %s\n" RESET, global_subnet_mask);
    printf(GREEN "Static DNS loaded: %s\n" RESET, global_dns_ip);

    signal(SIGINT, handle_signal_interrupt);
    init_ip_pool();

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        printf(RED "Socket creation failed.\n" RESET);
        exit(0);
    }
    printf(GREEN "Socket created successfully.\n" RESET);

    // Generate the gateway IP dynamically
    generate_dynamic_gateway_ip(global_gateway_ip, sizeof(global_gateway_ip));
    printf(GREEN "Dynamic Gateway generated: %s\n" RESET, global_gateway_ip);

    memset(&server_addr, 0, sizeof(server_addr));       // Zero out the structure
    server_addr.sin_family = AF_INET;                   // IPv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);     // Bind to any address on the system
    server_addr.sin_port = htons(port);                 // Convert port to network byte order

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf(RED "Socket bind failed.\n" RESET);
        close(sockfd);
        exit(0);
    }
    printf(GREEN "Socket bind successful.\n" RESET);
    printf(YELLOW "UDP server is running on %s:%d...\n" RESET, server_ip, port);

    // Create a thread to check and release expired leases
    pthread_t lease_thread;
    if (pthread_create(&lease_thread, NULL, check_and_release, NULL) != 0)
    {
        printf(RED "Failed to create leases thread.\n" RESET);
        end_program();
    }

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);

        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (recv_len < 0)
        {
            printf(RED "Failed to receive data.\n" RESET);
            continue;
        }

        client_data_t *client_data = (client_data_t *)malloc(sizeof(client_data_t));
        if (!client_data)
        {
            printf(RED "Failed to allocate memory for client data.\n" RESET);
            continue;
        }

        client_data->sockfd = sockfd;
        memcpy(client_data->buffer, buffer, BUFFER_SIZE);
        client_data->client_addr = client_addr;
        client_data->client_addr_len = client_addr_len;

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, process_client_connection, (void *)client_data) != 0)
        {
            printf(RED "Failed to create thread.\n" RESET);
            free(client_data);
            continue;
        }

        pthread_detach(thread_id);
    }

    end_program();
    return 0;
}
