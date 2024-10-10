// Essential includes for c programs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#include "./config/db.h"
#include <time.h>  // Para generar números aleatorios con srand() y rand()
#include "./data/ip_pool.h"


// Define the socket variable in a global scope so that it can be accessed by the signal handler
int sockfd;

// Declaración de funciones
void send_dhcpoffer(int socket_fd, struct sockaddr_in* client_addr, dhcp_message_t* discover_message);


void end_program()
{
    // Close the socket just if it was created
    if (sockfd >= 0)
    {
        close(sockfd);
    }
    printf("Exiting...\n");
    exit(0);
}

void handle_signal_interrupt(int signal)
{
    printf("\n");
    printf("Signal %d received.\n", signal);

    // Call the function to close the socket and exit the program
    end_program();
}

void *proccess_client_connection(void *arg)
{
    client_data_t *data = (client_data_t *)arg;
    char *buffer = data->buffer;
    struct sockaddr_in client_addr = data->client_addr;
    socklen_t client_addr_len = data->client_addr_len;
    int connection_sockfd = data->sockfd;

    printf("Processing DHCP message from client %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Create a DHCP message structure to hold the parsed data
    dhcp_message_t dhcp_msg;

    // Call the parse function to convert the raw buffer into the dhcp_message_t structure
    if (parse_dhcp_message((uint8_t *)buffer, &dhcp_msg) != 0)
    {
        printf("Failed to parse DHCP message.\n");
        free(data);
        return NULL;
    }

    // Call the print function to display the parsed message
    print_dhcp_message(&dhcp_msg);

    // Variable to store the message type (option 53 in DHCP options)
    uint8_t dhcp_message_type = 0;

    // Parse the options to find the DHCP message type (option 53)
    for (int i = 0; i < DHCP_OPTIONS_LENGTH; i++) {
        if (dhcp_msg.options[i] == 53) {  // Option 53 is for DHCP message type
            dhcp_message_type = dhcp_msg.options[i + 1];  // El siguiente byte es el tipo de mensaje
            break;
        }
    }

    // Verificar si el mensaje es un DHCP_DISCOVER
    if (dhcp_message_type == DHCP_DISCOVER)
    {

        // Enviar DHCPOFFER en respuesta
        send_dhcpoffer(connection_sockfd, &client_addr, &dhcp_msg);
    }

    // Example: Send a basic response (this is just a placeholder, actual DHCP response would be more complex)
    const char *response = "DHCP server response";
    sendto(connection_sockfd, response, strlen(response), 0, (SOCKET_ADDRESS *)&client_addr, client_addr_len);

    // Free the client_data_t structure that was dynamically allocated
    free(data);

    return NULL;
}


int main(int argc, char *argv[]) 
{

    srand(time(NULL));

    // Define a structure to hold the server address information
    struct sockaddr_in server_addr, client_addr;
    // String buffer to hold incoming messages
    char buffer[BUFFER_SIZE];
    // Define a structure to hold the client address information
    socklen_t client_addr_len = sizeof(client_addr);

    // Load environment variables
    load_env_variables();
    init_db();

    // Register the signal handler for SIGINT (CTRL+C)
    signal(SIGINT, handle_signal_interrupt);

    init_ip_pool();  // Inicializar el pool de IPs

    // Initialize the created socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // AF_INET: IPv4, SOCK_DGRAM: UDP

    // Check if the socket was created successfully
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
    memset(&server_addr, 0, sizeof(server_addr));       // Zero out the structure
    server_addr.sin_family = AF_INET;                   // IPv4
    server_addr.sin_addr.s_addr = inet_addr(server_ip); // Accept connections from the specified server IP not other IPS or interfaces
    server_addr.sin_port = htons(port);                 // Convert port number to network byte order (port 67 for DHCP server)

    // Bind the socket to the port
    if (bind(sockfd, (SOCKET_ADDRESS *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Socket Bind failed.\n");
        close(sockfd);
        exit(0);
    }
    else
    {
        printf("Socket bind successful.\n");
    }

    printf("UDP server is running on %s:%d...\n", server_ip, port);

    // Infinite loop to keep the server running, opens threads to handle each incoming client
    while (1)
    {
        // Clear the buffer before receiving a new message
        memset(buffer, 0, BUFFER_SIZE);

        // Receive a message from the client
        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (SOCKET_ADDRESS *)&client_addr, &client_addr_len);
        if (recv_len < 0)
        {
            printf("Failed to receive data.\n");
            continue;
        }

        // Dynamically allocate memory for client data
        client_data_t *client_data = (client_data_t *)malloc(sizeof(client_data_t));
        if (client_data == NULL)
        {
            printf("Failed to allocate memory for client data.\n");
            continue;
        }

        // Populate the client_data structure
        client_data->sockfd = sockfd;
        memcpy(client_data->buffer, buffer, BUFFER_SIZE);
        client_data->client_addr = client_addr;
        client_data->client_addr_len = client_addr_len;

        // Create a thread to handle the client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, proccess_client_connection, (void *)client_data) != 0)
        {
            printf("Failed to create thread.\n");
            free(client_data);
            continue;
        }

        // Detach the thread so that its resources are automatically reclaimed when it finishes
        pthread_detach(thread_id);
    }

    // Call the function to close the socket and exit the program
    end_program();
    return 0;
}

// Función para generar una Gateway IP aleatoria dentro del rango 192.168.1.[1-254]
void generate_dynamic_gateway_ip(char* gateway_ip, size_t size) {
    int last_octet = (rand() % 254) + 1;  // Genera un número entre 1 y 254
    snprintf(gateway_ip, size, "192.168.1.%d", last_octet);
}

void send_dhcpoffer(int socket_fd, struct sockaddr_in* client_addr, dhcp_message_t* discover_message) {
    dhcp_message_t offer_message;

    // Inicializar el mensaje DHCP
    init_dhcp_message(&offer_message);

    // Copiar la dirección MAC del cliente desde discover_message
    if (discover_message->hlen == 6) {  // Verificar que la longitud de la MAC es correcta
        memcpy(offer_message.chaddr, discover_message->chaddr, 6);  // Copiar los 6 bytes de la MAC
    } else {
        printf("Error: Longitud de la dirección MAC no válida.\n");
        return;
    }

    // Asignar una IP al cliente
    char* assigned_ip = assign_ip();  // Llamar a la función para asignar una IP
    if (assigned_ip == NULL) {
        printf("No IP available to offer to client.\n");
        return;
    }

    // Asignar la IP al campo 'yiaddr' (Your IP)
    inet_pton(AF_INET, assigned_ip, &offer_message.yiaddr);

    // Asignar la IP del servidor al campo 'siaddr' (Server IP)
    inet_pton(AF_INET, server_ip, &offer_message.siaddr);

    // Dejar la IP del cliente 'ciaddr' como 0.0.0.0
    inet_pton(AF_INET, "0.0.0.0", &offer_message.ciaddr);

    // Generar la Gateway IP dinámica
    char dynamic_gateway_ip[16];  // Buffer para la Gateway IP
    generate_dynamic_gateway_ip(dynamic_gateway_ip, sizeof(dynamic_gateway_ip));  // Generar la IP dinámica
    
    // Asignar la Gateway IP dinámica
    inet_pton(AF_INET, dynamic_gateway_ip, &offer_message.giaddr);

    // Establecer el tipo de mensaje DHCP a DHCPOFFER (opción 53)
    offer_message.options[0] = 53;
    offer_message.options[1] = 1;
    offer_message.options[2] = DHCP_OFFER;

    // Agregar máscara de subred (opción 1)
    offer_message.options[3] = 1;
    offer_message.options[4] = 4;
    inet_pton(AF_INET, "255.255.255.0", &offer_message.options[5]); // Máscara de subred

    // Fin de las opciones (Opción 255: end)
    offer_message.options[9] = 255;

    // Imprimir el mensaje DHCP antes de enviarlo para ver las IPs y la dirección MAC
    printf("DHCPOFFER message being sent:\n");
    print_dhcp_message(&offer_message);
    
    // Enviar el mensaje DHCPOFFER al cliente
    int bytes_sent = sendto(socket_fd, &offer_message, sizeof(offer_message), 0, (struct sockaddr *)client_addr, sizeof(*client_addr));

    if (bytes_sent == -1) {
        perror("Error sending DHCPOFFER");
    }
}
