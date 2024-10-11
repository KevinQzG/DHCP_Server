#include "client.h"
#include "./config/env.h"
#include "./data/message.h"
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

// Global socket descriptor
int sockfd;

// Function to retrieve the MAC address of a network interface (cross-platform)
int get_mac_address(uint8_t *mac, const char *iface)
{
#ifdef _WIN32
    PIP_ADAPTER_INFO adapterInfo;
    DWORD bufferSize = sizeof(IP_ADAPTER_INFO);
    adapterInfo = (IP_ADAPTER_INFO *)malloc(bufferSize);

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_BUFFER_OVERFLOW)
    {
        free(adapterInfo);
        adapterInfo = (IP_ADAPTER_INFO *)malloc(bufferSize);
    }

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR)
    {
        PIP_ADAPTER_INFO adapter = adapterInfo;
        while (adapter)
        {
            if (strcmp(adapter->AdapterName, iface) == 0)
            {
                memcpy(mac, adapter->Address, adapter->AddressLength);
                free(adapterInfo);
                return 0;
            }
            adapter = adapter->Next;
        }
    }
    free(adapterInfo);
    printf(RED "Failed to find MAC address for interface %s\n" RESET, iface);
    return -1;

#elif __APPLE__
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_dl *sdl;

    if (getifaddrs(&ifap) != 0)
    {
        perror("getifaddrs failed");
        return -1;
    }

    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr->sa_family == AF_LINK && strcmp(ifa->ifa_name, iface) == 0)
        {
            sdl = (struct sockaddr_dl *)ifa->ifa_addr;
            memcpy(mac, LLADDR(sdl), 6);
            freeifaddrs(ifap);
            return 0;
        }
    }

    freeifaddrs(ifap);
    printf(RED "Failed to find MAC address for interface %s\n" RESET, iface);
    return -1;

#else
    struct ifreq ifr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0)
    {
        perror(RED "Socket creation failed" RESET);
        return -1;
    }

    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
    {
        perror(RED "Failed to get MAC address" RESET);
        close(fd);
        return -1;
    }

    close(fd);

    memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
    return 0;
#endif
}

void end_program()
{
    if (sockfd >= 0)
    {
        close(sockfd);
    }
    printf(MAGENTA "Exiting...\n" RESET);
    exit(0);
}

void handle_signal_interrupt(int signal)
{
    printf(YELLOW "\nSignal %d received.\n" RESET, signal);
    end_program();
}

void handle_dhcp_offer(int sockfd, struct sockaddr_in *server_addr, dhcp_message_t *offer_msg)
{
    // Create message DHCP_REQUEST
    dhcp_message_t request_msg;
    init_dhcp_message(&request_msg);
    set_dhcp_message_type(&request_msg, DHCP_REQUEST);

    request_msg.siaddr = offer_msg->siaddr;
    request_msg.yiaddr = offer_msg->yiaddr;

    memcpy(request_msg.chaddr, offer_msg->chaddr, sizeof(request_msg.chaddr));

    uint8_t buffer[sizeof(dhcp_message_t)];
    build_dhcp_message(&request_msg, buffer, sizeof(buffer));

    printf(CYAN "Sending DHCP_REQUEST to the server...\n" RESET);
    if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0)
    {
        perror(RED "Error sending DHCP_REQUEST" RESET);
    }
    else
    {
        printf(GREEN "DHCP_REQUEST message sent to the server.\n" RESET);
    }

    // Wait for DHCP_ACK from the server
    if (recv_dhcp_ack(sockfd, server_addr) == 0)
    {
        printf(GREEN "Process completed successfully.\n" RESET);
    }
    else
    {
        printf(RED "Error processing DHCP_ACK.\n" RESET);
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
            // Verificar el tipo de mensaje DHCP
            if (received_msg.options[2] == DHCP_ACK)
            {
                struct in_addr assigned_ip;
                assigned_ip.s_addr = ntohl(received_msg.yiaddr);
                printf(GREEN BOLD "DHCP_ACK received. Assigned IP: %s\n" RESET, inet_ntoa(assigned_ip));
                return 0; // Success
            }
            else if (received_msg.options[2] == DHCP_DECLINE)
            {
                printf(RED BOLD "DHCP_DECLINE received: No IP addresses available.\n" RESET);
                return -1; // Error: no IPs disponibles
            }

            else
            {
                printf(YELLOW "Unexpected message type: %d\n" RESET, received_msg.options[2]);
            }
        }
        else
        {
            printf(RED "Failed to parse DHCP message.\n" RESET);
        }
    }
    else
    {
        printf(RED "Error receiving DHCP message.\n" RESET);
    }
    return -1; // Error
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
        printf(RED "Socket creation failed.\n" RESET);
        exit(0);
    }
    else
    {
        printf(GREEN "Socket created successfully.\n" RESET);
    }

    // Set the bytes in memory for the server_addr structure to 0
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    dhcp_message_t msg;
    uint8_t buffer[sizeof(dhcp_message_t)];

    // Initialize DHCP Discover message
    init_dhcp_message(&msg);
    set_dhcp_message_type(&msg, DHCP_DISCOVER);

    // Set the correct network interface for each OS
    const char *iface;
#ifdef _WIN32
    iface = "Ethernet";
#elif __APPLE__
    iface = "en0";
#else
    iface = "eth0";
#endif

    // Retrieve and set the client's MAC address in the DHCP message
    if (get_mac_address(msg.chaddr, iface) == 0)
    {
        msg.hlen = 6;
    }
    else
    {
        printf(RED "Failed to set MAC address.\n" RESET);
        close(sockfd);
        return -1;
    }

    // Serialize the message to a buffer
    build_dhcp_message(&msg, buffer, sizeof(buffer));

    // Send DHCP Discover message to the server
    int sent_bytes = sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, addr_len);
    if (sent_bytes < 0)
    {
        printf(RED "Failed to send message to server.\n" RESET);
        close(sockfd);
        return -1;
    }

    printf(CYAN "DHCP Discover message sent to server.\n" RESET);

    // DHCP_OFFER message received from the server
    memset(buffer, 0, sizeof(buffer));
    recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);

    if (recv_len > 0)
    {
        dhcp_message_t received_msg;
        if (parse_dhcp_message(buffer, &received_msg) == 0)
        {

            if (received_msg.options[2] == DHCP_OFFER)
            {
                struct in_addr offered_ip;
                offered_ip.s_addr = ntohl(received_msg.yiaddr);
                printf(BLUE "DHCP_OFFER received. IP offered: %s\n" RESET, inet_ntoa(offered_ip));

                // Process the DHCP_OFFER message from the server and send a DHCP_REQUEST
                handle_dhcp_offer(sockfd, &server_addr, &received_msg);
            }
            else
            {
                printf(YELLOW "Unexpected message type: %d\n" RESET, received_msg.options[2]);
            }
        }
        else
        {
            printf(RED "Failed to parse received message.\n" RESET);
        }
    }

    end_program();
    return 0;
}
