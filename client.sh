#!/bin/bash

# Step 1: Load environment variables from .env file
if [ -f .env ]; then
    # Source the .env file to handle variables with spaces correctly
    set -a    # Automatically export all variables
    source .env.client
    set +a    # Stop automatically exporting variables
else
    echo ".env.client file not found!"
fi

# Step 2: Create and navigate to the build directory
echo "Setting up build directory..."
mkdir -p bin

# Step 3: Compile the client code
echo "Compiling DHCP client..."
gcc -o bin/client ./src/client.c ./src/config/env.c ./src/data/message.c ./src/utils/utils.c ./src/data/ip_pool.c -lpthread

# Step 4: Run the client
echo "Running DHCP client..."
./bin/client #& # The & runs the client in the background

# Optional: Wait for client process to complete, if necessary
wait

# Step 6: Script end
echo "Client execution completed."
