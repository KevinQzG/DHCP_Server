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
#include "./data/message.h"
#include "data/ip_pool.h"

// Global variables
int sockfd;
char global_gateway_ip[16]; // Global variable for the gateway IP

client_record_t clients[MAX_CLIENTS];


// Function to check if a client request is a duplicate
int is_duplicate_request(uint32_t requested_ip)
{
    time_t now = time(NULL);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        // Check if the IP is already assigned
        if (clients[i].ip == requested_ip)
        {
            if (difftime(now, clients[i].timestamp) < 10)
            {
                return 1; // Duplicate
            }
            else
            {
                // Update timestamp if more than 10 seconds have passed
                clients[i].timestamp = now;
                return 0; // Not a duplicate
            }
        }
    }

    // If the IP is not in use, store the new one
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].timestamp == 0)
        {
            clients[i].ip = requested_ip; // Store the IP
            clients[i].timestamp = now;   // Set the timestamp
            return 0;                     // Not a duplicate
        }
    }

    return 0; // Not a duplicate
}

int main(int argc, char *argv[])
{
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
    server_addr.sin_addr.s_addr = inet_addr(server_ip); // Server IP
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

void *check_and_release(void *arg)
{
    while (1)
    {
        check_leases();
        sleep(1);
    }
}

void *process_client_connection(void *arg)
{
    client_data_t *data = (client_data_t *)arg;
    char *buffer = data->buffer;
    struct sockaddr_in client_addr = data->client_addr;
    socklen_t client_addr_len = data->client_addr_len;
    int connection_sockfd = data->sockfd;

    printf(CYAN "Processing DHCP message from client %s:%d\n" RESET, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    dhcp_message_t dhcp_msg;

    if (parse_dhcp_message((uint8_t *)buffer, &dhcp_msg) != 0)
    {
        printf(RED "Failed to parse DHCP message.\n" RESET);
        free(data);
        return NULL;
    }

    // Print the DHCP message with detailed formatting
    print_dhcp_message(&dhcp_msg, false);

    uint8_t dhcp_message_type = 0;
    for (int i = 0; i < DHCP_OPTIONS_LENGTH; i++)
    {
        if (dhcp_msg.options[i] == 53)
        {
            dhcp_message_type = dhcp_msg.options[i + 2]; // Corregido: obtener el valor real del tipo de mensaje
            break;
        }
    }

    switch (dhcp_message_type)
    {
    case DHCP_DISCOVER:
        printf(GREEN "Received DHCP_DISCOVER from client.\n" RESET);
        send_dhcpoffer(connection_sockfd, &client_addr, &dhcp_msg);
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

void send_dhcpoffer(int socket_fd, struct sockaddr_in *client_addr, dhcp_message_t *discover_message) {
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

        // End of options
        offer_message.options[15] = 255;
    }
    print_dhcp_message(&offer_message, false);

    // Send DHCP_OFFER or DHCP_NAK message
    if (sendto(sockfd, &offer_message, sizeof(offer_message), 0, (struct sockaddr *) client_addr, sizeof(*client_addr)) < 0) {
        perror(RED "Error sending DHCP message" RESET);
    } else {
        printf(CYAN "DHCP message sent to client.\n" RESET);
    }
}

// Function to check if a requested IP is available
int is_ip_available(uint32_t requested_ip)
{
    for (int i = 0; i < pool_size; i++)
    {
        char ip_buffer[16];
        int_to_ip(requested_ip, ip_buffer);
        if (strcmp(ip_pool[i].ip_address, ip_buffer) == 0 && ip_pool[i].is_assigned == 1)
        {
            return 0; // IP is already assigned, not available
        }
    }
    return 1;
}

// Function to renew the lease of an IP address
void renew_lease(char *ip_address)
{
    for (int i = 0; i < pool_size; i++)
    {
        if (strcmp(ip_pool[i].ip_address, ip_address) == 0)
        {
            ip_pool[i].lease_start = time(NULL);
            printf(GREEN "Lease renewed for IP address %s\n" RESET, ip_address);
            break;
        }
    }
}

void handle_dhcp_request(int sockfd, struct sockaddr_in *client_addr, dhcp_message_t *request_msg)
{
    dhcp_message_t response_msg;
    init_dhcp_message(&response_msg);
    memcpy(response_msg.chaddr, request_msg->chaddr, 6);
    inet_pton(AF_INET, server_ip, &response_msg.siaddr);

    int option_count = 3; // Declare option_count here

    // Check if the client is requesting an IP that is no longer available or if there is an error in the request
    if (!is_ip_available(request_msg->yiaddr))
    {
        // Send a DHCP_NAK if the requested IP is unavailable
        printf(RED "Requested IP is not available, sending DHCP_NAK...\n" RESET);
        set_dhcp_message_type(&response_msg, DHCP_NAK); // Set message type to DHCP_NAK
    }
    else if (is_duplicate_request(request_msg->yiaddr))
    {
        printf(YELLOW "Duplicate message detected, ignoring...\n" RESET);
        set_dhcp_message_type(&response_msg, DHCP_NAK);
    }
    else
    {
        renew_lease(inet_ntoa(*(struct in_addr *)&request_msg->yiaddr));
        printf(GREEN "Sending DHCP_ACK...\n" RESET);
        set_dhcp_message_type(&response_msg, DHCP_ACK); // Set message type to DHCP_ACK
        response_msg.yiaddr = request_msg->yiaddr;      // Assign the requested IP to the client

        is_duplicate_request(response_msg.yiaddr); // Call the function to register the IP

        // Add DNS (option 6) only here
        response_msg.options[option_count++] = 6;                               // Option 6: DNS
        response_msg.options[option_count++] = 4;                               // Length of DNS option (4 bytes)
        inet_pton(AF_INET, global_dns_ip, &response_msg.options[option_count]); // DNS Server IP
        option_count += 4;                                                      // Increment by 4 for the 4 bytes of the DNS IP
    }

    // Use the same gateway as in the DHCPOFFER
    inet_pton(AF_INET, global_gateway_ip, &response_msg.giaddr);

    // Ensure correct placement of the end option
    response_msg.options[option_count++] = 1;                                    // Option 1: Subnet Mask
    response_msg.options[option_count++] = 4;                                    // Length of Subnet Mask option (4 bytes)
    inet_pton(AF_INET, global_subnet_mask, &response_msg.options[option_count]); // Set the Subnet Mask
    option_count += 4;                                                           // Increment by 4 for the 4 bytes of the subnet mask

    // Add end option marker to the options field
    response_msg.options[option_count] = 255; // Option 255 marks the end

    // Print the DHCP message before sending
    print_dhcp_message(&response_msg, false); // This will now show the DNS as part of the ACK

    if (sendto(sockfd, &response_msg, sizeof(response_msg), 0, (struct sockaddr *)client_addr, sizeof(*client_addr)) < 0)
    {
        perror(RED "Error sending DHCP message" RESET);
    }
    else if (response_msg.options[2] == DHCP_ACK)
    {
        printf(GREEN "DHCP_ACK sent to client.\n" RESET);
    }
    else if (response_msg.options[2] == DHCP_NAK)
    {
        printf(RED "DHCP_NAK sent to client: IP not available.\n" RESET);
    }
}

void handle_dhcp_release(int sockfd, dhcp_message_t *release_msg)
{
    // Free the IP address
    release_ip(inet_ntoa(*(struct in_addr *)&release_msg->ciaddr));

    // Print the DHCP_RELEASE message
    printf("IP address %s released.\n", inet_ntoa(*(struct in_addr *)&release_msg->ciaddr));
}

void handle_signal_interrupt(int signal)
{
    printf("\nSignal %d received.\n", signal);
    end_program();
}

// Function to generate a dynamic gateway IP based on the first IP of the range
void generate_dynamic_gateway_ip(char *gateway_ip, size_t size)
{
    // Get the dynamic gateway IP from the IP pool
    strncpy(gateway_ip, get_gateway_ip(), size);
}

// Function to clean up and terminate the program
void end_program()
{
    if (sockfd >= 0)
        close(sockfd);
    if (ip_pool)
        free(ip_pool);
    printf("Exiting...\n");
    exit(0);
}
