// env.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./env.h"

#define MAX_CHARACTERS 360

char server_ip[MAX_CHARACTERS];
int port;

void load_env_variables() {
    // Get the PORT, SERVERIP through environment variables
    const char *port_env = getenv("PORT");
    const char *server_ip_env = getenv("SERVERIP");

    if (!port_env || !server_ip_env) {
        printf("The PORT and SERVERIP environment variables must be set.\n");
        exit(0);
    }

    port = atoi(port_env);  // Convert the port to an integer
    strcpy(server_ip, server_ip_env);  // Copy the server_ip_env to the server_ip variable
}
