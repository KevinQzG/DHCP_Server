#ifndef IP_POOL_H
#define IP_POOL_H

#include <stdint.h>  // Para usar uint32_t
#include <time.h>    // Para usar time_t

#define IP_ADDRESS_SIZE 16  // Tamaño de una dirección IP
// #define LEASE_TIME 1800     // Lease time in seconds (30 minutes)
#define LEASE_TIME 60
extern int pool_size;  // Declaración del tamaño del pool

int is_ip_available(uint32_t requested_ip);

// Estructura para manejar las direcciones IP
typedef struct {
    char ip_address[IP_ADDRESS_SIZE];  // Ip address as a string
    int is_assigned;      // Flag to indicate if the IP is assigned
    time_t lease_start;   // Timestamp when the lease was assigned
    int lease_duration;   // Lease duration in seconds
} ip_pool_entry_t;

// Declaración del pool de IPs (no especificamos el tamaño aquí, será dinámico)
extern ip_pool_entry_t* ip_pool;

// Funciones para manejar el pool de IPs
void init_ip_pool();  // Inicializa el pool de IPs
char* assign_ip();    // Asigna una IP del pool disponible
void release_ip(const char* ip);  // Libera una IP asignada
char* get_gateway_ip();  // Nueva declaración
void check_leases();  // Function to check and release expired leases

// Declaración de funciones para convertir IP a entero y viceversa
unsigned int ip_to_int(const char* ip);
void int_to_ip(unsigned int ip, char* buffer);

// Función para calcular el tamaño del pool de direcciones IP basado en el rango dinámico
int calculate_pool_size(char* start_ip, char* end_ip);

#endif
