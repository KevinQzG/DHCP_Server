#!/bin/bash

# Step 1: Load environment variables from .env file
if [ -f .env ]; then
    export $(grep -v '^#' .env | xargs)
else
    echo ".env file not found!"
    exit 1
fi

# Step 2: Create and navigate to the build directory
echo "Setting up build directory..."
mkdir -p bin

# Step 3: Compile the client code
echo "Compiling DHCP client..."
gcc -o bin/client ./src/client.c ./src/config/env.c

# Step 4: Run the client
echo "Running DHCP client..."
./bin/client #& # The & runs the client in the background

# Optional: Wait for client process to complete, if necessary
wait

# Step 6: Script end
echo "Client execution completed."
