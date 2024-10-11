// env.h
#ifndef ENV_H
#define ENV_H

#define MAX_CHARACTERS_IP 360
#define MAX_CHARACTERS_PATH 520

extern char server_ip[];
extern int port;
extern char ip_range[MAX_CHARACTERS_PATH];
extern char global_dns_ip[16];


// Function to load environment variables
void load_env_variables();

#endif
