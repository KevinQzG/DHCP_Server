#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./env.h"

char server_ip[MAX_CHARACTERS_IP];
int port;
char ip_range[MAX_CHARACTERS_PATH];
char global_dns_ip[IP_ADDRESS_SIZE];
char global_subnet_mask[IP_ADDRESS_SIZE];

void load_env_variables() {
    // Get the PORT, SERVER_IP, SUBNET, DNS, and IP_RANGE through environment variables
    const char *port_env = getenv("PORT");
    const char *server_ip_env = getenv("SERVER_IP");
    const char *ip_range_env = getenv("IP_RANGE");  // Nueva variable de entorno
    const char *dns_env = getenv("DNS");
    const char *subnet_env = getenv("SUBNET");


    if (!port_env || !ip_range_env || !dns_env || !subnet_env) {
        printf("The PORT, IP_RANGE, DNS, and SUBNET environment variables are required.\n");
        exit(0);
    }

    port = atoi(port_env);  // Convert the port to an integer
    if (!server_ip_env) {
        strcpy(server_ip, "");
    } else {
        strcpy(server_ip, server_ip_env);
    }
     // Copy the server_ip_env to the server_ip variable
    strcpy(ip_range, ip_range_env);  // Copy the ip_range_env to the ip_range variable
    strcpy(global_dns_ip, dns_env);  // Copy the dns_env to the global_dns_ip variable
    strcpy(global_subnet_mask, subnet_env);  // Copy the subnet_env to the global_subnet_mask variable
}
