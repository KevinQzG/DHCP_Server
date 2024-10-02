# DHCP_Server

## Content Table
- [Developers](#developers)
- [Introduction](#introduction)
- [Project Structure](#project-structure)
- [Execution](#execution)
- [License](#license)
- [Contact](#contact)

## Developers
Juan Felipe Restrepo Buitrago (Main)
Kevin Quiroz González
El Manin

## Introduction
This project involves the development of a functional DHCP server in C and a client. The server dynamically assigns IPv4 addresses and network parameters to clients within a local or remote network using the Berkeley Sockets API.

## Project Structure

.
├── src \ # Source files
│   ├── __init__.py # API initialization. \
|   ├── config/ # Configuration files
|   |   ├── env.c # Environment configuration file
|   |   └── env.h # Environment configuration header file
|   ├── client.c # Client source code
|   ├── server.c # Server source code
|   ├── client.h # Client header file
|   └── server.h # Server header file
├── .gitignore # Git ignore file
├── README.md # Project README file
└── LICENSE # Project license file

## Execution

1. **Environment Configuration**: Make sure you have C, the GCC compiler installed on your machine. If not, you can install it by running the following command:

```bash
sudo apt-get install gcc
```

2. **Clone the Repository**: Clone the repository to your local machine.

3. **Environment Variables**: Configure the environment variables as in the `.env.example` file.

4. **Project's Root Directory**: Navigate to the project's root directory.

5. **Compile the Server**: Compile the server by running the following command:

```bash
gcc -o bin/server ./src/server.c ./src/config/env.c -lpthread
```

6. **Compile the Client**: Compile the client by running the following command:

```bash
gcc -o bin/client ./src/client.c ./src/config/env.c
```

7. **Run the Server**: Run the server by executing the following command:

```bash
./bin/server
```

8. **Run the Client**: Run the client by executing the following command:

```bash
./bin/client
```

## Testing

To test the server or the client you can execute the server.sh or client.sh scripts respectively. These scripts will compile the server or the client and run them. You just need to follow the next steps:

1. **.env File Configuration**: Make sure you have the `.env` file configured with the desired environment variables in the project root directory.

2. **Server Testing**: To test the server, run the following command:

```bash
./server.sh
```

3. **Client Testing**: To test the client, run the following command:

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