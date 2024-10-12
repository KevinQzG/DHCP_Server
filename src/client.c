// Personal includes
#include "./client.h"
#include "./config/env.h"
#include "./data/message.h"
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

// Define the socket variable in a global scope so that it can be accessed by the signal handler
int sockfd;
dhcp_message_t assigned_values_msg;
struct sockaddr_in server_addr;
int is_ip_assigned = 0; // Global flag to track IP assignment


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

int main() {
    socklen_t addr_len = sizeof(server_addr);
    int recv_len;

    // Load environment variables
    load_env_variables();

    // Register the signal handler for SIGINT (CTRL+C)
    signal(SIGINT, handle_signal_interrupt);

    // Initialize the created socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // AF_INET: IPv4, SOCK_DGRAM: UDP

    if (sockfd < 0) {
        printf("Socket creation failed.\n");
        exit(0);
    } else {
        printf("Socket created successfully.\n");
    }

    // Set the bytes in memory for the server_addr structure to 0
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip); // Dirección del servidor DHCP
    server_addr.sin_port = htons(port);                 // Puerto 67 para DHCP

    dhcp_message_t msg;
    uint8_t buffer[sizeof(dhcp_message_t)];

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
        printf("Failed to set MAC address.\n");
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

    // Recibir el DHCP_OFFER del servidor
    memset(buffer, 0, sizeof(buffer));
    recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);

    if (recv_len > 0) {
        dhcp_message_t received_msg;
        if (parse_dhcp_message(buffer, &received_msg) == 0) {
            // Verifies if the message is DHCP_OFFER
            if (received_msg.options[2] == DHCP_OFFER) {
                struct in_addr offered_ip;
                offered_ip.s_addr = ntohl(received_msg.yiaddr);
                printf("DHCP_OFFER received. Offered IP: %s\n", inet_ntoa(offered_ip));

                // Process the DHCP_OFFER message from the server and send a DHCP_REQUEST
                handle_dhcp_offer(sockfd, &server_addr, &received_msg);
            } 
        } else {
             printf(RED "Failed to parse received message.\n" RESET);
        }
    }

    // Wait for the user to end the program with Ctrl+C while renewing the lease
    while (is_ip_assigned) {
        sleep(1);
    }

    // Call the function to close the socket and end the program
    end_program();
    return 0;
}

void handle_dhcp_offer(int sockfd, struct sockaddr_in *server_addr, dhcp_message_t *offer_msg) {
    // Crear mensaje DHCP_REQUEST
    dhcp_message_t request_msg;
    init_dhcp_message(&request_msg);
    set_dhcp_message_type(&request_msg, DHCP_REQUEST);

    request_msg.siaddr = offer_msg->siaddr;
    request_msg.yiaddr = offer_msg->yiaddr;

    memcpy(request_msg.chaddr, offer_msg->chaddr, sizeof(request_msg.chaddr));

    uint8_t buffer[sizeof(dhcp_message_t)];
    build_dhcp_message(&request_msg, buffer, sizeof(buffer));

    printf(CYAN "Sending DHCP_REQUEST to the server...\n" RESET);
    if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror(RED "Error sending DHCP_REQUEST" RESET);
    } else {
        printf(GREEN "DHCP_REQUEST message sent to the server.\n" RESET);
    }


    // Wait for DHCP_ACK from the server
    if (recv_dhcp_ack(sockfd, server_addr) == 0) {
        printf(GREEN "Process completed successfully.\n" RESET);
    } else {
        printf(RED "Error processing DHCP_ACK.\n" RESET);
    }
}

int recv_dhcp_ack(int sockfd, struct sockaddr_in *server_addr) {
    uint8_t buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(*server_addr);
    int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)server_addr, &addr_len);

    if (recv_len > 0) {
        dhcp_message_t received_msg;
        if (parse_dhcp_message(buffer, &received_msg) == 0) {
            // Verificar que el tipo de mensaje es DHCP_ACK
            if (received_msg.options[2] == DHCP_ACK) {
                struct in_addr assigned_ip;

                assigned_ip.s_addr = ntohl(received_msg.yiaddr); // Conversión de red a host
                printf(GREEN "DHCP_ACK received. Assigned IP: %s\n" RESET, inet_ntoa(assigned_ip));

                assigned_values_msg = received_msg;
                // Assign the assigned IP to the client address in the message
                assigned_values_msg.ciaddr = received_msg.yiaddr;
                is_ip_assigned = 1; // Set the flag to indicate IP assignment

                return 0;           // Éxito
            } else if (received_msg.options[2] == DHCP_DECLINE) {             
                printf(RED BOLD "DHCP_DECLINE received: No IP addresses available from the server.\n" RESET);
                return -1;
            } 
        } else {
            printf(RED "Failed to parse DHCP_ACK message.\n" RESET);
        }
    } else {
        printf(RED "Error receiving DHCP_ACK.\n" RESET);
    }
    return -1; // Error
}

void send_dhcp_release(int sockfd, struct sockaddr_in *server_addr) {
    // Reuse the global variable 'assigned_values_msg'
    // The ciaddr field should already contain the client's assigned IP address

    // Update the DHCP options to indicate a DHCP_RELEASE message type
    assigned_values_msg.options[0] = 53;           // Option 53: DHCP message type
    assigned_values_msg.options[1] = 1;            // Length of the option
    assigned_values_msg.options[2] = DHCP_RELEASE; // Set DHCP message type to DHCP_RELEASE

    // Send the DHCP Release message
    int msg_len = sizeof(dhcp_message_t); // Use the actual message size
    if (sendto(sockfd, &assigned_values_msg, msg_len, 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("Failed to send DHCP_RELEASE message");
    } else {
        printf("DHCP_RELEASE message sent.\n");
    }
}
