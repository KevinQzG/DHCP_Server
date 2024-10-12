// Personal includes
#include "./client.h"
#include "./utils/utils.h"

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
int sockfd;
dhcp_message_t assigned_values_msg;
struct sockaddr_in server_addr;
int is_ip_assigned = 0; // Global flag to track IP assignment
int renewal_time;
time_t lease_start_time;

pthread_mutex_t ip_assignment_mutex = PTHREAD_MUTEX_INITIALIZER;


void send_dhcp_release(int sockfd, struct sockaddr_in *server_addr) {
    // Reuse the global variable 'assigned_values_msg'
    // The ciaddr field should already contain the client's assigned IP address

    // Update the DHCP options to indicate a DHCP_RELEASE message type
    assigned_values_msg.options[0] = 53;           // Option 53: DHCP message type
    assigned_values_msg.options[1] = 1;            // Length of the option
    assigned_values_msg.options[2] = DHCP_RELEASE; // Set DHCP message type to DHCP_RELEASE

    // Serialize the message to a buffer
    uint8_t buffer[sizeof(dhcp_message_t)];
    build_dhcp_message(&assigned_values_msg, buffer, sizeof(buffer));

    // Send the DHCP_RELEASE message to the server
    if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror(RED "Error sending DHCP_RELEASE" RESET);
    } else {
        printf(CYAN "DHCP_RELEASE message sent to the server.\n" RESET);
    }
}


void end_program() {
    // Release the assigned IP with DHCP_RELEASE
    if (is_ip_assigned) {
        send_dhcp_release(sockfd, &server_addr);
    } else {
        printf("No IP address assigned. Skipping DHCP_RELEASE.\n");
    }

    // Close the socket if it is open
    if (sockfd >= 0) {
        close(sockfd);
    }

    printf(MAGENTA "Exiting...\n" RESET);
    exit(0);
}


void handle_signal_interrupt(int signal) {
    printf(YELLOW "\nSignal %d received.\n" RESET, signal);
    end_program();
}


void send_dhcp_request(int sockfd, struct sockaddr_in *server_addr, dhcp_message_t *msg) {
    // Set the message type to DHCP_REQUEST
    set_dhcp_message_type(msg, DHCP_REQUEST);

    // Serialize the message to a buffer
    uint8_t buffer[sizeof(dhcp_message_t)];
    build_dhcp_message(msg, buffer, sizeof(buffer));

    // Send the DHCP Request message to the server
    if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror(RED "Error sending DHCP_REQUEST" RESET);
    } else {
        printf(CYAN "DHCP_REQUEST message sent to the server.\n" RESET);
    }
}


void handle_dhcp_offer(int sockfd, struct sockaddr_in *server_addr, dhcp_message_t *msg) {
    // Set the message type to DHCP_REQUEST
    set_dhcp_message_type(msg, DHCP_REQUEST);

    send_dhcp_request(sockfd, server_addr, msg);   
}


void renew_lease(int sockfd, struct sockaddr_in *server_addr) {
    printf("Renewing lease...\n");

    if (!is_ip_assigned) {
        printf(RED "No IP address assigned. Skipping lease renewal.\n" RESET);
        return;
    }

    // Send DHCP REQUEST to renew lease
    send_dhcp_request(sockfd, server_addr, &assigned_values_msg);

    // Update lease start time after successful renewal
    lease_start_time = time(NULL);
}


// Function to listen for DHCP Offer/ACK in a separate thread
void *dhcp_listener(void *arg) {
    socklen_t addr_len = sizeof(server_addr);
    char buffer[BUFFER_SIZE];
    dhcp_message_t msg;

    while (1) {
        // Clean the buffer
        memset(buffer, 0, sizeof(buffer));
        // Listen for incoming DHCP OFFER or ACK
        int recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (recv_len < 0) {
            printf(RED "Failed to receive data.\n" RESET);
            continue;
        }

        // Parse the incoming result
        int parse_result = parse_dhcp_message((uint8_t *)buffer, &msg);
        if (parse_result != 0){
            printf(RED "Failed to parse received message.\n" RESET);
            continue;
        }

        // Print the DHCP message with detailed formatting
        print_dhcp_message(&msg, true);
        
        uint8_t dhcp_message_type = 0;
        for (int i = 0; i < DHCP_OPTIONS_LENGTH; i++) {
            if (msg.options[i] == 53) {
                dhcp_message_type = msg.options[i + 2]; // Corregido: obtener el valor real del tipo de mensaje
                break;
            }
        }

        switch (dhcp_message_type) {
            case DHCP_OFFER:
                // Lock the mutex
                pthread_mutex_lock(&ip_assignment_mutex);

                struct in_addr offered_ip;
                offered_ip.s_addr = ntohl(msg.yiaddr);
                printf(GREEN "DHCP_OFFER received. Offered IP: %s\n" RESET, inet_ntoa(offered_ip));

                // Process the DHCP_OFFER message from the server and send a DHCP_REQUEST
                handle_dhcp_offer(sockfd, &server_addr, &msg);
            
                // Unlock the mutex
                pthread_mutex_unlock(&ip_assignment_mutex);
                break;
            case DHCP_ACK:
                // Lock the mutex
                pthread_mutex_lock(&ip_assignment_mutex);

                struct in_addr assigned_ip;
                assigned_ip.s_addr = ntohl(msg.yiaddr);
                printf(GREEN "DHCP_ACK received. Assigned IP: %s\n" RESET, inet_ntoa(assigned_ip));

                assigned_values_msg = msg;
                // Assign the assigned IP to the client address in the message
                assigned_values_msg.ciaddr = msg.yiaddr;
                is_ip_assigned = 1; // Set the flag to indicate IP assignment

                // Set the lease start time
                lease_start_time = time(NULL);

                // Unlock the mutex
                pthread_mutex_unlock(&ip_assignment_mutex);
                break;
            case DHCP_NAK:
                printf(RED "DHCP_NACK received: The ip was not assigned.\n" RESET);
                is_ip_assigned = 0;
                break;
            default:
                printf(RED "Unrecognized DHCP message type: %d\n" RESET, dhcp_message_type);
                break;
        }
    }

    return NULL;
}


int main() {
    socklen_t addr_len = sizeof(server_addr);
    int recv_len;
    dhcp_message_t msg;
    // uint8_t buffer[sizeof(dhcp_message_t)];
    char buffer[BUFFER_SIZE];

    // Load environment variables
    load_env_variables();

    // Register the signal handler for SIGINT (CTRL+C)
    signal(SIGINT, handle_signal_interrupt);

    renewal_time = LEASE_TIME / 2;

    // Initialize the created socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // AF_INET: IPv4, SOCK_DGRAM: UDP

    if (sockfd < 0) {
        printf(RED "Socket creation failed.\n" RESET);
        exit(0);
    } else {
        printf(GREEN "Socket created successfully.\n" RESET);
    }

    // Set the bytes in memory for the server_addr structure to 0
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip); 
    server_addr.sin_port = htons(port);      

    // Create a thread to listen for DHCP Offer/ACK messages
    pthread_t listener_thread;
    if (pthread_create(&listener_thread, NULL, dhcp_listener, NULL) != 0) {
        printf(RED "Failed to create listener thread.\n" RESET);
        end_program();
    }

    time_t start_discover_time = time(NULL);

    while (1) {
        time_t current_discover_time = time(NULL);

        pthread_mutex_lock(&ip_assignment_mutex);
        if (current_discover_time - start_discover_time >= 30 && is_ip_assigned == 0) {

            // Initialize DHCP Discover message
            init_dhcp_message(&msg);
            set_dhcp_message_type(&msg, DHCP_DISCOVER);

            // Set the correct network interface for each OS
            const char *iface;
        #ifdef _WIN32
            iface = "Ethernet"; // Nombre común en Windows, cambiar según sea necesario
        #elif __APPLE__
            iface = "en0"; // Interfaz típica en macOS
        #else
            iface = "eth0"; // O cambiar a "enp3s0" según tu sistema
        #endif

            // Retrieve and set the client's MAC address in the DHCP message
            if (get_mac_address(msg.chaddr, iface) == 0) {
            msg.hlen = 6; // Longitud de la dirección MAC
            } else {
                printf(RED "Failed to get MAC address.\n" RESET);
                close(sockfd);
                return -1;
            }

            // Serialize the message to a buffer
            build_dhcp_message(&msg, buffer, sizeof(buffer));

            // Send DHCP Discover message to the server
            int sent_bytes = sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, addr_len);
            if (sent_bytes < 0) {
                printf(RED "Failed to send message to server.\n" RESET);
                close(sockfd);
                return -1;
            }

            printf(CYAN "DHCP Discover message sent to server.\n" RESET);

            start_discover_time = time(NULL);
        } else if (is_ip_assigned) {
            // Renew the lease after 30 seconds
            time_t current_time = time(NULL);
            int time_elapsed = (int)difftime(current_time, lease_start_time);

            if (time_elapsed >= renewal_time) {
                // Renew the lease
                renew_lease(sockfd, &server_addr);
            }
        }
        pthread_mutex_unlock(&ip_assignment_mutex);
        
        sleep(1);
    }

    // Call the function to close the socket and end the program
    end_program();
    return 0;
}
