#include "ip_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>    // Para usar time_t

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

    // Change the gateway IP in the pool to assigned
    strncpy(ip_pool[0].ip_address, gateway_ip, sizeof(ip_pool[0].ip_address));
    ip_pool[0].is_assigned = 1;

    // Convertir las IPs de inicio y fin a enteros
    unsigned int start = ip_to_int(start_ip) + 1;  // Start from the second IP
    unsigned int end = ip_to_int(end_ip);

    int i = 1;
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
        if (ip_pool[i].is_assigned == 0) {
            ip_pool[i].is_assigned = 1;     // Marks the IP as assigned

            // Assign IP in the DHCP Offer/Ack phase
            time_t current_time = time(NULL);  // Get the current time

            ip_pool[i].lease_start = current_time;  // Record lease start time
            ip_pool[i].lease_duration = LEASE_TIME;  // Assign lease duration

            return ip_pool[i].ip_address;   // Return the IP address
        }
    }
    return NULL;  // In case no IP is available
}


void release_ip(const char* ip) {
    for (int i = 0; i < pool_size; i++) {
        if (strcmp(ip_pool[i].ip_address, ip) == 0) {
            ip_pool[i].is_assigned = 0;  // Marks the IP as available
            return;
        }
    }
    printf("IP not found in pool: %s\n", ip);
}

void check_leases() {
    time_t current_time = time(NULL);
    
    for (int i = 1; i < pool_size; i++) {
        if (ip_pool[i].is_assigned) {
            // Check if the lease has expired
            if ((current_time - ip_pool[i].lease_start) >= ip_pool[i].lease_duration) {
                printf("Lease for IP %s has expired. Releasing IP...\n", ip_pool[i].ip_address);
                ip_pool[i].is_assigned = 0;  // Mark IP as free
            }
        }
    }
}

