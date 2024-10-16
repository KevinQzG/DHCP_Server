#!/bin/bash

# Step 1: Load environment variables from .env file
if [ -f .env ]; then
    # Source the .env file to handle variables with spaces correctly
    set -a    # Automatically export all variables
    source .env
    set +a    # Stop automatically exporting variables
else
    echo ".env file not found!"
fi

# Step 2: Create and navigate to the build directory
echo "Setting up build directory..."
mkdir -p bin

# Step 3: Compile the server code
echo "Compiling DHCP server..."
gcc -o bin/relay ./src/relay.c ./src/config/env.c -lpthread

# Step 4: Run the relay
echo "Running DHCP relay..."
echo ""
./bin/relay

# Step 6: Script end
echo "Relay execution completed."
