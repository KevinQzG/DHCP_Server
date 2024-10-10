#ifndef IP_POOL_H
#define IP_POOL_H

// Definir el tamaño del pool de direcciones IP
#define POOL_SIZE (255 * 245)  // Aumenta el tamaño del pool según el rango que necesites

// Estructura para manejar las direcciones IP
typedef struct {
    char ip_address[16];  // Dirección IP en formato de texto
    int is_assigned;      // Estado de la IP: 0 = libre, 1 = asignada
} ip_pool_entry_t;

// Declaración del pool de IPs
extern ip_pool_entry_t ip_pool[POOL_SIZE];

// Funciones para manejar el pool de IPs
void init_ip_pool();  // Inicializa el pool de IPs
char* assign_ip();    // Asigna una IP del pool disponible

#endif
