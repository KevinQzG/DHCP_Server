#include "ip_pool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Definir el pool de IPs
ip_pool_entry_t ip_pool[POOL_SIZE];

// Función para inicializar el pool de IPs
void init_ip_pool() {
    char ip_base[16];
    int subnet = 1;  // Empezar con la primera subred (puedes cambiarlo según necesites)

    for (int i = 0; i < POOL_SIZE; i++) {
        // Crear la base de la dirección IP (subred variable)
        snprintf(ip_base, sizeof(ip_base), "192.168.%d.", subnet);
        
        // Generar direcciones IP secuenciales dentro de la subred
        int last_octet = (i % 255) + 1;  // Generar el último octeto
        snprintf(ip_pool[i].ip_address, sizeof(ip_pool[i].ip_address), "%s%d", ip_base, last_octet);
        
        // Marcar la IP como no asignada
        ip_pool[i].is_assigned = 0;
        
        // Cambiar de subred después de llenar 255 IPs
        if (last_octet == 255) {
            subnet++;
        }
    }
}

// Función para asignar una IP
char* assign_ip() {
    for (int i = 0; i < POOL_SIZE; i++) {
        if (ip_pool[i].is_assigned == 0) {  // Si la IP está libre
            ip_pool[i].is_assigned = 1;     // Marcarla como asignada
            return ip_pool[i].ip_address;   // Retornar la IP asignada
        }
    }
    return NULL;  // Si no hay IPs disponibles
}
