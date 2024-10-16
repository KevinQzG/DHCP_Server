# DHCP_Server

## Content Table
- [Developers](#developers)
- [Introduction](#introduction)
- [Development](#development)
- [Milestones Achieved and Not Achieved](#milestones-achieved-and-not-achieved)
  - [Server](#server)
  - [Client](#client)
  - [Additional Features](#additional-features)
- [Project Structure](#project-structure)
- [Diagram](#diagram)
- [Execution](#execution)
- [Execution with Docker for Relay Testing](#execution-with-docker-for-relay-testing)
- [Video](#video)
- [References](#references)
- [Conclusion](#conclusion)
- [License](#license)
- [Contact](#contact)

## Developers
- [Juan Felipe Restrepo Buitrago](https://github.com/JuanFelipeRestrepoBuitrago)
- [Kevin Quiroz González](https://github.com/KevinQzG)
- [Jacobo Zuluaga](https://github.com/jacobozj)

## Introduction

This project involves the development of a functional DHCP server and client in C. The server dynamically assigns IPv4 addresses and network parameters to clients within a local or remote network using the Berkeley Sockets API.

The Dynamic Host Configuration Protocol (DHCP) is a network protocol used to assign IP addresses and provide configuration information to devices on a network. The DHCP server is responsible for managing a pool of IP addresses and assigning them to clients that request them. The server also provides additional network parameters such as the subnet mask, default gateway, and DNS server.

The process of assigning an IP address to a client involves the following steps: 

- **1. DHCP Discover**: The client sends a broadcast message to the network requesting an IP address.
- **2. DHCP Offer**: The server responds with a broadcast message offering an IP address to the client.
- **3. DHCP Request**: The client sends a broadcast message requesting the offered IP address.
- **4.1. DHCP Acknowledge**: The server responds with a broadcast message acknowledging the IP address assignment.
- **4.2. DHCP Nak**: The server responds with a broadcast message denying the IP address assignment.
- **5. IP Address Lease**: The client uses the assigned IP address for a specified period of time and then renews the lease sending a new request to the server.
- **6. IP Address Release**: The client releases the IP address when it is no longer needed.

## Development

At the beginning of the project, the team defined the project structure and  the features that the server and client should have. Additionally, we look for information in the internet about the Berkeley Sockets API and the DHCP protocol to understand how to implement the server and client. We first understood the basic concepts and functionality of the DHCP protocol, such as the DHCP message structure, the different types of messages, and the options that can be included in the messages.

After understanding the DHCP protocol and what processes the server and client should perform, we started the implementation of the server and client. To work on the project, we divided the tasks among the team members according to their available time and skills. We used the C programming language to implement the server and client, and the Berkeley Sockets API to establish communication between the server and client.

The server and client were implemented using the following steps:

- **1. Server Initialization**: A simple UDP socket was created to listen for incoming DHCP messages from clients.
- **2. Client Initialization**: A simple UDP socket was created to send DHCP messages to the server.
- **3. Environment Configuration**: The server and client were configured to read environment variables from a `.env` and a `.env.client` file, respectively.
- **4. Message Structure**: The DHCP message structure was defined to include the different fields and options required by the protocol.
- **5. IP Pool Management**: A pool of IP addresses was added to the server to assign to clients requesting an IP address.
- **6. DHCP Message Handling**: The server and client were implemented to handle the different types of DHCP messages (Discover, Offer, Request, Acknowledge, Nak).
- **7. IP Address Assignment**: A mechanism was implemented to assign an IP address to a client requesting an IP address.
- **8. IP Address Lease**: The server was implemented to lease an IP address to a client for a specified period of time. Renewal and release of the IP address were also implemented first for server and then for client.
- **9. Server and Client Execution**: The server and client were implemented to run as separate processes and communicate with each other using the Berkeley Sockets API.
- **10. Broadcast Messages**: The server and client were implemented to send and receive broadcast messages to communicate with each other within a local network.
- **11. Testing**: The server and client were tested to verify that they were working correctly and assigning IP addresses to clients as expected.
- **12. AWS EC2 Deployment**: The server and client were deployed on AWS EC2 instances to test communication between a local server and a remote client.

## Milestones Achieved and Not Achieved

We managed to implement the server and client to assign IP addresses to clients using the DHCP protocol. Both applications were implemented in C language using the Berkeley Sockets API to establish communication between the server and client.

They work as expected, where the server assigns IP addresses to clients that request them, and the client receives the assigned IP address and network parameters from the server. Below are the milestones achieved during the development of the project:

### Server
- [x] **Server Listening**: The server listens for incoming DHCP messages from clients on a UDP socket, both on local and remote networks.
- [x] **IP Address Assignment**: The server dynamically assigns IP addresses to clients from a pool of available IP addresses when requested.
- [x] **IP Pool Management**: The server manages a pool of IP addresses created from a range of IPs defined by the user through environment variables.
- [x] **IP Address Lease Management**: The server leases an IP address to a client for a specified period. It handles the renewal and release of the IP address either when the client requests it or when the lease expires.
- [x] **Simultaneous Clients**: The server supports multiple clients simultaneously by using threads to process incoming DHCP messages from clients concurrently.
- [x] **DHCP Message Handling**: The server processes the primary DHCP message types, including Discover, Offer, Request, Acknowledge, Nak, and prints the received messages for logging purposes.
![Message Printing for Server](./public/server_print.png)
- [x] **IP Lease Logging**: The server logs every assigned IP address, along with the lease time and client details, for future reference.
- [x] **Error Management**: The server handles errors gracefully by printing error messages and exiting the program when an error occurs or sending a Nak message to the client when the IP address assignment fails.
- [x] **Cross-Subnet Client Handling**: The server can handle clients from different subnets by using a relay agent to forward DHCP messages between the client and server.

### Client

- [x] **Client Implementation**: We manage to implement a client in C language. 
- [x] **Requesting IP Address**: The client sends a DHCP Discover message to the server to request an IP address.
- [x] **Receiving IP Address**: The client receives an IP address and network parameters from the server in a DHCP Offer message. Furthermore, the client prints the received IP address and network parameters in the console. The elements printed are: the client IP address, the offered IP address, the server IP address, the subnet mask, the default gateway, the DNS server, the client MAC address and the lease time in seconds.
![Message Printing for Client](./public/client_print.png)
- [x] **IP Address Lease Management**: The client manages the lease of the assigned IP address by renewing the lease with the server when the lease time is about to expire.
- [x] **IP Address Release**: The client releases the assigned IP address when it is no longer needed by sending a DHCP Release message to the server. The Release message is sent when the execution of the client is finished. 

### Additional Features

- [x] **Server and Client Broadcast Messages**: The server and client communicate with each other using broadcast messages to send and receive DHCP messages within a local network.
- [x] **RELEASE and NAK Messages**: The client sends a RELEASE message to the server when it is finished executing to release the assigned IP address. The server sends a NAK message to the client when the IP address assignment fails.

## Suggestions for Future Work

- **Security**: Implement security features to protect the communication between the server and client, such as encryption and authentication mechanisms.
- **Relay Agent**: Upgrade the relay agent to handle multiple clients from different subnets simultaneously. Implement a mechanism to save incoming messages from clients in a queue and process them concurrently to avoid message loss.
- **Logging**: Implement a logging mechanism to save the server and client logs in a file for future reference.

## Project Structure

.   
├── src \ # Source files    
|   ├── config/ # Configuration files   
|   |   ├── env.c # Environment configuration file  
|   |   └── env.h # Environment configuration header file   
|   ├── data/ # Data files  
|   |   ├── ip_pool.c # Management of the IP pool   
|   |   ├── ip_pool.h # IP pool header file     
|   |   ├── message.c # Management of the DHCP messages and its structure   
|   |   └── message.h # DHCP message header file    
|   ├── utils/ # Utility files  
|   |   ├── utils.c # Utility functions   
|   |   └── utils.h # Utility header file   
|   ├── relay.c # Relay source code    
|   ├── relay.h # Relay header file    
|   ├── client.c # Client source code   
|   ├── server.c # Server source code   
|   ├── client.h # Client header file   
|   └── server.h # Server header file   
├── .env.example # Environment variables template   
├── .dockerignore # Docker ignore file    
├── Dockerfile # Docker file    
├── client.sh # Client execution script     
├── server.sh # Server execution script    
├── relay.sh # Relay execution script    
├── .gitignore # Git ignore file    
├── README.md # Project README file     
└── LICENSE # Project license file      

## Diagram
This diagram shows the interaction between the main components of a DHCP server.

![SoWkIImgAStDuNBEoKpDAr7GjLC8JYqgIorIi59myN0EK739B4xE1_Av8C8W1H2bC0N-QMb0MLpQWr8BIrEBIt3gTapEpiilpqbDIKzLq5VmIyp6Or9EQc9nge96Va59PdvUOeucbqDgNWhGxG00](https://github.com/user-attachments/assets/096951b4-16b7-41cf-b703-ae69664387fa)

![VPD1Qnin48Nl-ok6zD1B0h53GYcbnBNhDj0q9ixD-31UZTTYjIDtfBNhblxtIbvHAzqG6GpQzytmtWowTHx5juqfti8NOrTem97tcYi7Qok0K1f21cG5E-rQDEO0kDqIci20iROkNArvYu8DhB2iAx4jgx4reVhDz7WoOXi8hEXJ3xoAlnT0AueaGSFnrUVhopuqjUmJCfZU3DvDNaRc-XV1u2OJXKgFN9bCpZsCHEm-DRuy](https://github.com/user-attachments/assets/3479d702-7d47-4d4e-9e99-4d3135b3aca4)




## Execution

1. **Environment Configuration**: Make sure you have C, the GCC compiler installed on your machine. If not, you can install it by running the following command:

```bash
sudo apt-get install gcc
```

2. **Clone the Repository**: Clone the repository to your local machine.


4. **Project's Root Directory**: Navigate to the project's root directory.

3. **.env File Configuration**: Make sure you have the `.env` file for server and `.env.client` for client configured with the desired environment variables in the project root directory. You can use the template in the `.env.example` file.

2. **Server Execution**: To run the server, run the following command:

```bash
./server.sh
```

3. **Client Execution**: To run the client, run the following command:

```bash
./client.sh
```

4. **Relay Execution**: To run the relay, run the following command:

```bash
./relay.sh
```

## Execution with Docker for Relay Testing

1. **Docker Installation**: Make sure you have Docker installed on your machine. If not, you can install it by following the instructions in the [official Docker documentation](https://docs.docker.com/get-docker/).

2. **Clone the Repository**: Clone the repository to your local machine.

3. **Project's Root Directory**: Navigate to the project's root directory.

5. **Docker Relay Build**: To build the Docker image for the relay, first change the 19 line in the Dockerfile for `CMD ["sh", "relay.sh"]` and then run the following command:

```bash
docker build . -t dhcp_relay_image
```

6. **Docker Client Build**: To build the Docker image for the client, first change the 19 line in the Dockerfile for `CMD ["sh", "client.sh"]` and then run the following command:

```bash
docker build . -t dhcp_client_image
```

7. **Docker Server Build**: To build the Docker image for the server, first change the 19 line in the Dockerfile for `CMD ["sh", "server.sh"]` and then run the following command:

```bash
docker build . -t dhcp_server_image
```

8. **Docker Network Creation**: To create a Docker network, run the following command:

```bash
docker network create --subnet=192.168.1.0/24 subnet1 && docker network create --subnet=192.168.2.0/24 subnet2
```

9. **Docker Server Run**: To run the server, run the following command:

```bash
docker run -it \
  --name dhcp-server \
  --net subnet1 \
  --ip 192.168.1.2 \
  -e PORT=8080 \
  -e SUBNET="255.255.255.0" \
  -e IP_RANGE="192.168.1.30-192.168.1.255" \
  -e DNS="8.8.8.8" \
  dhcp_server_image
```

10. **Docker Client Run**: To run the client, run the following command:

```bash
docker run -it \
  --name dhcp-client \
  --net subnet2 \
  --ip 192.168.2.2 \
  -e PORT=8080 \
  -e SUBNET="0.0.0.0" \
  -e IP_RANGE="0.0.0.0-0.0.0.0" \
  -e DNS="0.0.0.0" \
  dhcp_client_image
```

11. **Docker Relay Run**: To run the relay, run the following command:

```bash
docker run -it \
  --name dhcp-relay \
  --net subnet1 \
  --net subnet2 \
  --cap-add=NET_ADMIN \
  -e PORT=8080 \
  -e SUBNET="0.0.0.0" \
  -e SERVER_IP="192.168.1.2" \
  -e IP_RANGE="0.0.0.0-0.0.0.0" \
  -e DNS="0.0.0.0" \
  dhcp_relay_image
```

## Video

- [Video de Sustentación del Proyecto](https://youtu.be/OrhwklPeCMY)

## References

- [Berkeley Sockets API](https://en.wikipedia.org/wiki/Berkeley_sockets)
- [Dynamic Host Configuation Protocol (DHCP) Message Options](https://www.omnisecu.com/tcpip/dhcp-dynamic-host-configuration-protocol-message-options.php#:~:text=Message%20Type%20indicates%20the%20DHCP%20message%20and%20can,Dynamic%20Host%20Configuration%20Protocol%20%28DHCP%29%20Request%20message%20%28DHCPRequest%29.)
- [RFC 2131: Dynamic Host Configuration Protocol](https://datatracker.ietf.org/doc/html/rfc2131)
- [Microsoft DHCP Server Documentation](https://docs.microsoft.com/en-us/windows-server/networking/technologies/dhcp/dhcp-top)
- [DHCP Fundamentals](https://www.networkworld.com/article/3239896/what-is-dhcp-and-how-does-it-work.html)
- [Cisco DHCP Configuration](https://www.cisco.com/c/en/us/td/docs/ios-xml/ios/ipaddr_dhcp/configuration/15-mt/dhcp-15-mt-book/config-dhcp-server.html)
- [RFC 3046: DHCP Relay Agent](https://datatracker.ietf.org/doc/html/rfc3046)
- [Cisco: DHCP Overview](https://www.cisco.com/c/en/us/td/docs/ios-xml/ios/ipaddr_dhcp/configuration/15-mt/ipaddr-dhcp-15-mt-book/ipaddr-dhcp-config-dhcp.html)
- [Microsoft: Configure DHCP Using Windows Server](https://learn.microsoft.com/en-us/windows-server/networking/technologies/dhcp/dhcp-deploy-windows-server)
- [Apple: About DHCP on macOS](https://support.apple.com/guide/mac-help/change-dhcp-settings-mchlp2591/mac)

## Conclusion

The project successfully created a DHCP server and client that allow IP addresses to be automatically assigned to devices within a network. Throughout the development, the team implemented key functions for the server to manage the connection of multiple clients at the same time, offering IPs and necessary network settings such as the gateway and DNS server. Although the project achieved all important goals, such as IP management and communication between subnets, there is still room for further improvement, especially in the areas of security and activity logging.


## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Contact

For any questions or issues, feel free to reach out to:
- Juan Felipe Restrepo Buitrago: [jfrestrepb@eafit.edu.co](mailto:jfrestrepb@eafit.edu.co)
- Kevin Quiroz González: [kquirozg@eafit.edu.co](mailto:kquirozg@eafit.edu.co)
- Jacobo Zuluaga: [jzuluagaj@eafit.edu.co](mailto:jzuluagaj@eafit.edu.co)
