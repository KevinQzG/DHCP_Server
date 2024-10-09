// env.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./env.h"


char server_ip[MAX_CHARACTERS_IP];
int port;
char db_path[MAX_CHARACTERS_PATH];

void load_env_variables() {
    // Get the PORT, SERVERIP through environment variables
    const char *port_env = getenv("PORT");
    const char *server_ip_env = getenv("SERVER_IP");
    const char *db_path_env = getenv("DB_PATH");

    if (!port_env || !server_ip_env || !db_path_env) {
        printf("The PORT, SERVER_IP and DB_PATH environment variables are not set.\n");
        exit(0);
    }

    port = atoi(port_env);  // Convert the port to an integer
    strcpy(server_ip, server_ip_env);  // Copy the server_ip_env to the server_ip variable
    strcpy(db_path, db_path_env);  // Copy the db_path_env to the db_path variable
}
