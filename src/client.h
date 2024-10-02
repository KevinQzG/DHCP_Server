#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>

#define MAX_CHARACTERS 360
#define BUFFER_SIZE 1024 // Buffer size for incoming messages, maximum size of a DHCP message is 1024 bytes
#define SOCKET_ADDRESS struct sockaddr // Define SOCKET_ADDRESS as struct sockaddr 

// Function to handle signal interrupt (Ctrl+C)
void handle_signal_interrupt(int signal);

// Function to end the program
void end_program();

#endif