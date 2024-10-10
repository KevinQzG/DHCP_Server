#!/bin/bash

# Step 1: Load environment variables from .env file
if [ -f .env ]; then
    # Source the .env file to handle variables with spaces correctly
    set -a    # Automatically export all variables
    source .env
    set +a    # Stop automatically exporting variables
else
    echo ".env file not found!"
    exit 1
fi

# Step 2: Create and navigate to the build directory
echo "Setting up build directory..."
mkdir -p bin

# Step 3: Compile the server code
echo "Compiling DHCP server..."
gcc -o bin/server ./src/server.c ./src/config/env.c ./src/config/db.c ./src/data/message.c  ./src/data/ip_pool.c -lpthread -lsqlite3

# Step 4: Run the server
echo "Running DHCP server..."
echo ""
./bin/server

# Step 6: Script end
echo "Server execution completed."
