//
// Created by thomas on 01/04/25.
//

#include <netdb.h>
#include "Networking.hpp"

Networking::Networking()
{
    my_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (my_socket < 0)
    {
        std::cerr << "Socket creation falied";
        return;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);


    if(bind(my_socket, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "bind failed";
        close(my_socket);
        return;
    }
    // Set the client address to the hard coded values
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(9999);
    if (inet_pton(AF_INET, "132.145.48.206", &client_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid client address" << std::endl;
    }

    listen_thread = std::thread(&Networking::Listen, this);
}

void Networking::Listen()
{
    while(true)
    {
        struct sockaddr_in this_client;
        socklen_t length = sizeof(this_client);
        int n = recvfrom(my_socket, buffer.data(), BUFFER_SIZE, 0, (struct sockaddr *)&this_client, &length);
        if (n < 0)
        {
            std::cerr << "Receive failed" << std::endl;
        }
        buffer[n] = '\0';
        std::cout << "Server: " << buffer.data() << std::endl;
        // add the data to the messages vector
        messages.push_back(buffer);
    }
}

