#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#elif __APPLE__
#include <net/if_dl.h>  // Para sockaddr_dl y la estructura de enlace de nivel de datos en macOS
#include <ifaddrs.h>    // Para obtener la lista de interfaces en macOS
#else
#include <linux/if_packet.h> // Para sockaddr_ll en Linux
#endif

// Personal includes
#include "./client.h"
#include "./config/env.h"
#include "./data/message.h"

// Define the socket variable in a global scope so that it can be accessed by the signal handler
int sockfd;

// Function to retrieve the MAC address of a network interface (cross-platform)
int get_mac_address(uint8_t *mac, const char *iface) {
#ifdef _WIN32
    // Código para Windows
    PIP_ADAPTER_INFO adapterInfo;
    DWORD bufferSize = sizeof(IP_ADAPTER_INFO);
    adapterInfo = (IP_ADAPTER_INFO *)malloc(bufferSize);

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
        free(adapterInfo);
        adapterInfo = (IP_ADAPTER_INFO *)malloc(bufferSize);
    }

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
        PIP_ADAPTER_INFO adapter = adapterInfo;
        while (adapter) {
            if (strcmp(adapter->AdapterName, iface) == 0) {
                memcpy(mac, adapter->Address, adapter->AddressLength);
                free(adapterInfo);
                return 0;
            }
            adapter = adapter->Next;
        }
    }
    free(adapterInfo);
    printf("Failed to find MAC address for interface %s\n", iface);
    return -1;

#elif __APPLE__
    // Código para macOS
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_dl *sdl;

    if (getifaddrs(&ifap) != 0) {
        perror("getifaddrs failed");
        return -1;
    }

    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_LINK && strcmp(ifa->ifa_name, iface) == 0) {
            sdl = (struct sockaddr_dl *)ifa->ifa_addr;
            memcpy(mac, LLADDR(sdl), 6);  // Copiar la dirección MAC
            freeifaddrs(ifap);  // Liberar la lista de interfaces
            return 0;
        }
    }

    freeifaddrs(ifap);  // Liberar la lista de interfaces
    printf("Failed to find MAC address for interface %s\n", iface);
    return -1;

#else
    // Código para Linux
    struct ifreq ifr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1); // Nombre de la interfaz (e.g., "eth0")
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {   // Obtener la dirección MAC
        perror("Failed to get MAC address");
        close(fd);
        return -1;
    }

    close(fd);

    // Copiar la MAC address en el buffer proporcionado
    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6); // 6 bytes de la dirección MAC
    return 0;
#endif
}

void end_program() {
    if (sockfd >= 0) {
        close(sockfd);
    }
    printf("Exiting...\n");
    exit(0);
}

void handle_signal_interrupt(int signal) {
    printf("\nSignal %d received.\n", signal);
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
    memset(&server_addr, 0, sizeof(server_addr));       // Zero out the structure
    server_addr.sin_family = AF_INET;                   // IPv4
    server_addr.sin_addr.s_addr = inet_addr(server_ip); // Accept connections from the specified server IP not other IPS or interfaces
    server_addr.sin_port = htons(port);                 // Convert port number to network byte order (port 67 for DHCP server)

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
        printf("Failed to send message to server.\n");
        close(sockfd);
        return -1;
    }

    printf("DHCP Discover message sent to server.\n");

    // Receive server's response
    memset(buffer, 0, sizeof(buffer)); // Esto asegura que memset limpie el tamaño correcto del buffer
    int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
    if (recv_len > 0) {
        printf("Server response: %s\n", buffer);
    } else {
        printf("Failed to receive response from server.\n");
    }

    // Call the function to close the socket and exit the program
    end_program();
    return 0;
}
