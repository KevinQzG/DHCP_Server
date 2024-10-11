#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./env.h"

char server_ip[MAX_CHARACTERS_IP];
int port;

// Nueva variable para almacenar el rango de IP
char ip_range[MAX_CHARACTERS_PATH];

void load_env_variables() {
    // Get the PORT, SERVER_IP, DB_PATH, and IP_RANGE through environment variables
    const char *port_env = getenv("PORT");
    const char *server_ip_env = getenv("SERVER_IP");
    const char *ip_range_env = getenv("IP_RANGE");  // Nueva variable de entorno

    if (!port_env || !server_ip_env  || !ip_range_env) {
        printf("The PORT, SERVER_IP, and IP_RANGE environment variables are not set.\n");
        exit(0);
    }

    port = atoi(port_env);  // Convert the port to an integer
    strcpy(server_ip, server_ip_env);  // Copy the server_ip_env to the server_ip variable
    strcpy(ip_range, ip_range_env);  // Copy the ip_range_env to the ip_range variable
}
