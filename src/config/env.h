// env.h
#ifndef ENV_H
#define ENV_H

// Define ANSI color codes
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define MAGENTA "\033[35m"
#define RED "\033[31m"

#define IP_ADDRESS_SIZE 16  // Size of an IP address
#define MAX_CHARACTERS_IP 360
#define MAX_CHARACTERS_PATH 520


extern char server_ip[];
extern int port;
extern char ip_range[];
extern char global_dns_ip[];
extern char global_subnet_mask[];


// Function to load environment variables
void load_env_variables();

#endif
