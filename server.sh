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

# Step 3: Compile the server code
echo "Compiling DHCP server..."
gcc -o bin/server ./src/server.c ./src/config/env.c ./src/data/message.c -lpthread

# Step 4: Run the server
echo "Running DHCP server..."
echo ""
./bin/server #& # The & runs the server in the background

# Optional: Wait for server process to complete, if necessary
wait

# Step 6: Script end
echo "Server execution completed."
