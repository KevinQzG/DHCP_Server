#include "./utils.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
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


// Function to retrieve the MAC address of a network interface (cross-platform)
int get_mac_address(uint8_t *mac, const char *iface) {
#ifdef _WIN32
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
    printf(RED "Failed to find MAC address for interface %s\n" RESET, iface);
    return -1;

#elif __APPLE__
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
    printf(RED "Failed to find MAC address for interface %s\n" RESET, iface);
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
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) { // Obtener la dirección MAC
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