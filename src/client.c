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
#include <net/if_dl.h>
#include <ifaddrs.h>
#else
#include <linux/if_packet.h>
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
            memcpy(mac, LLADDR(sdl), 6);
            freeifaddrs(ifap);
            return 0;
        }
    }

    freeifaddrs(ifap);
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

    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("Failed to get MAC address");
        close(fd);
        return -1;
    }

    close(fd);
    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
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

    if (sockfd < 0) {
        printf("Socket creation failed.\n");
        exit(0);
    } else {
        printf("Socket created successfully.\n");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    dhcp_message_t msg;
    uint8_t buffer[sizeof(dhcp_message_t)];

    // Initialize DHCP Discover message
    init_dhcp_message(&msg);
    set_dhcp_message_type(&msg, DHCP_DISCOVER);

    const char *iface;
#ifdef _WIN32
    iface = "Ethernet";
#elif __APPLE__
    iface = "en0";
#else
    iface = "eth0";
#endif

    if (get_mac_address(msg.chaddr, iface) == 0) {
        msg.hlen = 6;
    } else {
        printf("Failed to set MAC address.\n");
        close(sockfd);
        return -1;
    }

    // Serialize the message to a buffer
    build_dhcp_message(&msg, buffer, sizeof(buffer));

    int sent_bytes = sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, addr_len);
    if (sent_bytes < 0) {
        printf("Failed to send message to server.\n");
        close(sockfd);
        return -1;
    }

    printf("DHCP Discover message sent to server.\n");

    // Receive server's response (DHCP Offer)
    memset(buffer, 0, sizeof(buffer));
    int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
    if (recv_len > 0) {
        printf("DHCP Offer received from server.\n");

        // Parse the DHCP Offer message
        dhcp_message_t offer_msg;
        if (parse_dhcp_message(buffer, &offer_msg) == 0) {
            // Prepare DHCP Request message
            dhcp_message_t request_msg;
            init_dhcp_message(&request_msg);
            set_dhcp_message_type(&request_msg, DHCP_REQUEST);

            // Set the MAC address
            memcpy(request_msg.chaddr, msg.chaddr, 6);
            request_msg.hlen = 6;

            // Set the IP offered by the server (yiaddr) in the DHCP Request message
            request_msg.ciaddr = offer_msg.yiaddr;
            request_msg.siaddr = offer_msg.siaddr;

            // Serialize the DHCP Request message to a buffer
            build_dhcp_message(&request_msg, buffer, sizeof(buffer));

            // Send DHCP Request message to the server
            sent_bytes = sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, addr_len);
            if (sent_bytes < 0) {
                printf("Failed to send DHCP Request message to server.\n");
                close(sockfd);
                return -1;
            }

            printf("DHCP Request message sent to server.\n");

            // Receive DHCP ACK from the server
            memset(buffer, 0, sizeof(buffer));
            recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
            if (recv_len > 0) {
                printf("DHCP ACK received from server. IP configuration successful.\n");
            } else {
                printf("Failed to receive DHCP ACK from server.\n");
            }
        } else {
            printf("Failed to parse DHCP Offer message.\n");
        }
    } else {
        printf("Failed to receive response from server.\n");
    }

    end_program();
    return 0;
}
