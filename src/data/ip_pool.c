#include "ip_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../config/env.h"

// Definir el pool de IPs como un puntero para que sea dinámico
ip_pool_entry_t* ip_pool = NULL;
int pool_size = 0;
char gateway_ip[16];  // Dirección IP del gateway (será la primera IP del rango)

// Función para calcular el tamaño del pool de direcciones IP basado en el rango dinámico
int calculate_pool_size(char* start_ip, char* end_ip) {
    unsigned int start = ip_to_int(start_ip);
    unsigned int end = ip_to_int(end_ip);
    return (end - start + 1);  // El tamaño del pool es la diferencia entre IP final e inicial
}

// Función para convertir una IP en formato texto a entero
unsigned int ip_to_int(const char* ip) {
    unsigned int a, b, c, d;
    sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d);
    return (a << 24) | (b << 16) | (c << 8) | d;
}

// Función para convertir un entero a una IP en formato texto
void int_to_ip(unsigned int ip, char* buffer) {
    sprintf(buffer, "%u.%u.%u.%u", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

// Función para inicializar el pool de IPs basado en el rango
void init_ip_pool() {
    // Usar la variable ip_range cargada desde el archivo .env
    if (strlen(ip_range) == 0) {
        printf("No IP range specified in .env\n");
        return;
    }

    // Dividir la IP de inicio y fin
    char start_ip[16], end_ip[16];
    sscanf(ip_range, "%[^-]-%s", start_ip, end_ip);

    // Calcular el tamaño del pool
    pool_size = calculate_pool_size(start_ip, end_ip);

    // Asignar memoria para el pool de IPs
    ip_pool = (ip_pool_entry_t*)malloc(pool_size * sizeof(ip_pool_entry_t));
    if (ip_pool == NULL) {
        printf("Failed to allocate memory for IP pool.\n");
        return;
    }

    // Asignar la primera IP del rango como gateway
    strncpy(gateway_ip, start_ip, sizeof(gateway_ip));

    // Convertir las IPs de inicio y fin a enteros
    unsigned int start = ip_to_int(start_ip);
    unsigned int end = ip_to_int(end_ip);

    int i = 0;
    char ip_buffer[16];

    // Generar todas las IPs dentro del rango y almacenarlas en el pool
    for (unsigned int ip = start; ip <= end && i < pool_size; ip++) {
        int_to_ip(ip, ip_buffer);
        strncpy(ip_pool[i].ip_address, ip_buffer, sizeof(ip_pool[i].ip_address));
        ip_pool[i].is_assigned = 0;  // Marcar como no asignada
        i++;
    }

    printf("Gateway IP set to: %s\n", gateway_ip);  // Mostrar la IP del gateway
}

char* get_gateway_ip() {
    return gateway_ip;  // Retornar la dirección del gateway
}



char* assign_ip() {
    for (int i = 0; i < pool_size; i++) {
        // Verificar que la IP no sea la misma que la del gateway
        if (ip_pool[i].is_assigned == 0 && strcmp(ip_pool[i].ip_address, get_gateway_ip()) != 0) {
            ip_pool[i].is_assigned = 1;     // Marcar la IP como asignada
            return ip_pool[i].ip_address;   // Retornar la IP asignada
        }
    }
    return NULL;  // Si no hay IPs disponibles
}

