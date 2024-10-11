#ifndef IP_POOL_H
#define IP_POOL_H
#include <stdint.h>  // Para usar uint32_t
extern int pool_size;  // Declaración del tamaño del pool



int is_ip_available(uint32_t requested_ip);


// Estructura para manejar las direcciones IP
typedef struct {
    char ip_address[16];  // Dirección IP en formato de texto
    int is_assigned;      // Estado de la IP: 0 = libre, 1 = asignada
} ip_pool_entry_t;

// Declaración del pool de IPs (no especificamos el tamaño aquí, será dinámico)
extern ip_pool_entry_t* ip_pool;

// Funciones para manejar el pool de IPs
void init_ip_pool();  // Inicializa el pool de IPs
char* assign_ip();    // Asigna una IP del pool disponible
char* get_gateway_ip();  // Nueva declaración

// Declaración de funciones para convertir IP a entero y viceversa
unsigned int ip_to_int(const char* ip);
void int_to_ip(unsigned int ip, char* buffer);

// Función para calcular el tamaño del pool de direcciones IP basado en el rango dinámico
int calculate_pool_size(char* start_ip, char* end_ip);

#endif
