cmake_minimum_required(VERSION 3.10)

# Project name
project(DHCPServer)

# Set the C standard
set(CMAKE_C_STANDARD 99)

# Source files
set(SOURCES ./src/main.c ./src/config/env.c)

# Find the environment variables
set(PORT_ENV $ENV{PORT})
set(SERVERIP_ENV $ENV{SERVERIP})

# Check if the environment variables are set
if(NOT PORT_ENV)
    message(FATAL_ERROR "PORT environment variable is not set")
endif()

if(NOT SERVERIP_ENV)
    message(FATAL_ERROR "SERVERIP environment variable is not set")
endif()

# Add definitions to be used in the code
add_definitions(-DPORT_ENV=${PORT_ENV} -DSERVERIP_ENV=\"${SERVERIP_ENV}\")

# Executable
add_executable(dhcp_server ${SOURCES})

# Include directories if necessary
target_include_directories(dhcp_server PRIVATE config)
