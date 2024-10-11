#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>

// Personal includes
#include "./client.h"
#include "./config/env.h"
#include "./data/message.h"
#include "./utils/utils.h"

#define RESET "\033[0m"
#define BOLD "\033[1m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define MAGENTA "\033[35m"
#define RED "\033[31m"

// Define the socket variable in a global scope so that it can be accessed by the signal handler
int sockfd;
dhcp_message_t assigned_values_msg;


void end_program()
{
    if (sockfd >= 0)
    {
        close(sockfd);
    }
    printf("Exiting...\n");
    exit(0);
}

void handle_signal_interrupt(int signal)
{
    printf("\nSignal %d received.\n", signal);
    end_program();
}

int main()
{
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    int recv_len;

    // Load environment variables
    load_env_variables();

    // Register the signal handler for SIGINT (CTRL+C)
    signal(SIGINT, handle_signal_interrupt);

    // Initialize the created socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // AF_INET: IPv4, SOCK_DGRAM: UDP

    if (sockfd < 0)
    {
        printf("Socket creation failed.\n");
        exit(0);
    }
    else
    {
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
    if (get_mac_address(msg.chaddr, iface) == 0)
    {
        msg.hlen = 6; // Longitud de la dirección MAC
    }
    else
    {
        printf("Failed to set MAC address.\n");
        close(sockfd);
        return -1;
    }

    // Serialize the message to a buffer
    build_dhcp_message(&msg, buffer, sizeof(buffer));

    // Send DHCP Discover message to the server
    int sent_bytes = sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, addr_len);
    if (sent_bytes < 0)
    {
        printf("Failed to send message to server.\n");
        close(sockfd);
        return -1;
    }

    printf("DHCP Discover message sent to server.\n");

    // Recibir el DHCP_OFFER del servidor
    memset(buffer, 0, sizeof(buffer));
    recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);

    if (recv_len > 0)
    {
        dhcp_message_t received_msg;
        if (parse_dhcp_message(buffer, &received_msg) == 0)
        {
            // Verificar que el tipo de mensaje es DHCP_OFFER
            if (received_msg.options[2] == DHCP_OFFER)
            {
                struct in_addr offered_ip;
                offered_ip.s_addr = ntohl(received_msg.yiaddr); // Conversión correcta de red a host
                printf("DHCP_OFFER recibido. IP ofrecida: %s\n", inet_ntoa(offered_ip));

                // Procesar el DHCP_OFFER y enviar el DHCP_REQUEST
                handle_dhcp_offer(sockfd, &server_addr, &received_msg);
            }
            else
            {
                printf("Tipo de mensaje no esperado: %d\n", received_msg.options[2]);
            }
        }
        else
        {
            printf("Fallo al parsear el mensaje recibido.\n");
        }
    }

    // Release the assigned IP with DHCP_RELEASE
    send_dhcp_release(sockfd, &server_addr);

    // Llamar a la función para cerrar el socket y salir del programa
    end_program();
    return 0;
}

void handle_dhcp_offer(int sockfd, struct sockaddr_in *server_addr, dhcp_message_t *offer_msg)
{
    // Crear mensaje DHCP_REQUEST
    dhcp_message_t request_msg;
    init_dhcp_message(&request_msg);
    set_dhcp_message_type(&request_msg, DHCP_REQUEST);

    // Copiar la dirección IP del servidor y la ofrecida
    request_msg.siaddr = offer_msg->siaddr;
    request_msg.yiaddr = offer_msg->yiaddr;

    // Usar la misma MAC que ya fue establecida
    memcpy(request_msg.chaddr, offer_msg->chaddr, sizeof(request_msg.chaddr));

    // Serializar y enviar el mensaje DHCP_REQUEST
    uint8_t buffer[sizeof(dhcp_message_t)];
    build_dhcp_message(&request_msg, buffer, sizeof(buffer));

    printf("Enviando DHCP_REQUEST al servidor...\n");
    if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0)
    {
        perror("Error al enviar DHCP_REQUEST");
    }
    else
    {
        printf("Mensaje DHCP_REQUEST enviado al servidor.\n");
    }

    // Esperar el DHCP_ACK del servidor
    if (recv_dhcp_ack(sockfd, server_addr) == 0)
    {
        printf("Proceso DHCP completado con éxito.\n");
    }
    else
    {
        printf("Error en el proceso DHCP al esperar DHCP_ACK.\n");
    }
}

int recv_dhcp_ack(int sockfd, struct sockaddr_in *server_addr)
{
    uint8_t buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(*server_addr);
    int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)server_addr, &addr_len);

    if (recv_len > 0)
    {
        dhcp_message_t received_msg;
        if (parse_dhcp_message(buffer, &received_msg) == 0)
        {
            // Verificar que el tipo de mensaje es DHCP_ACK
            if (received_msg.options[2] == DHCP_ACK)
            {
                struct in_addr assigned_ip;
                assigned_ip.s_addr = ntohl(received_msg.yiaddr); // Conversión de red a host
                printf(GREEN "DHCP_ACK received. Assigned IP: %s\n" RESET, inet_ntoa(assigned_ip));
                assigned_values_msg = received_msg;
                // Assign the assigned IP to the client address in the message
                assigned_values_msg.ciaddr = received_msg.yiaddr;
                return 0; // Éxito
            }
            else
            {
                printf(RED "Unexpected message type: %d\n" RESET, received_msg.options[2]);
            }
        }
        else
        {
            printf(RED "Failed to parse DHCP_ACK message.\n" RESET);
        }
    }
    else
    {
        printf(RED "Error receiving DHCP_ACK.\n" RESET);
    }
    return -1; // Error
}

void send_dhcp_release(int sockfd, struct sockaddr_in *server_addr) {
    // Reuse the global variable 'assigned_values_msg'
    // The ciaddr field should already contain the client's assigned IP address
    
    // Update the DHCP options to indicate a DHCP_RELEASE message type
    assigned_values_msg.options[0] = 53;  // Option 53: DHCP message type
    assigned_values_msg.options[1] = 1;  // Length of the option
    assigned_values_msg.options[2] = DHCP_RELEASE;  // Set DHCP message type to DHCP_RELEASE

    // Send the DHCP Release message
    int msg_len = sizeof(dhcp_message_t);  // Use the actual message size
    if (sendto(sockfd, &assigned_values_msg, msg_len, 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("Failed to send DHCP_RELEASE message");
    } else {
        printf("DHCP_RELEASE message sent.\n");
    }
}

