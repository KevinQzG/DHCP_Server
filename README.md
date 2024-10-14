# DHCP_Server

## Content Table
- [Developers](#developers)
- [Introduction](#introduction)
- [Project Structure](#project-structure)
- [Execution](#execution)
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
|   ├── client.c # Client source code   
|   ├── server.c # Server source code   
|   ├── client.h # Client header file   
|   └── server.h # Server header file   
├── .env.example # Environment variables template   
├── client.sh # Client execution script     
├── server.sh # Server execution script     
├── .gitignore # Git ignore file    
├── README.md # Project README file     
└── LICENSE # Project license file      

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

## References

- [Berkeley Sockets API](https://en.wikipedia.org/wiki/Berkeley_sockets)
- [Dynamic Host Configuation Protocol (DHCP) Message Options](https://www.omnisecu.com/tcpip/dhcp-dynamic-host-configuration-protocol-message-options.php#:~:text=Message%20Type%20indicates%20the%20DHCP%20message%20and%20can,Dynamic%20Host%20Configuration%20Protocol%20%28DHCP%29%20Request%20message%20%28DHCPRequest%29.)

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Contact

For any questions or issues, feel free to reach out to:
- Juan Felipe Restrepo Buitrago: [jfrestrepb@eafit.edu.co](mailto:jfrestrepb@eafit.edu.co)
- Kevin Quiroz González: [kquirozg@eafit.edu.co](mailto:kquirozg@eafit.edu.co)
- El Manin: [manin@eafit.edu.co](mailto:manin@eafit.edu.co)