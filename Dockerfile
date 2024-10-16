# Use the official Ubuntu image as the base
FROM ubuntu:latest

# Set the working directory
WORKDIR /app

# Install the necessary dependencies for building the C program
RUN apt-get update \
    && apt-get install -y gcc make build-essential libc-dev linux-headers-generic iproute2 iputils-ping \
    && apt-get clean

# Copy the current directory contents into the container at /app
COPY . /app

# Expose port 8080 for UDP communication
EXPOSE 8080/udp

# Run the specific script
CMD ["sh", "relay.sh"]
