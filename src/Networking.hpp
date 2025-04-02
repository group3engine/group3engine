//
// Created by thomas on 01/04/25.
//

#ifndef GROUP3ENGINE_NETWORKING_HPP
#define GROUP3ENGINE_NETWORKING_HPP
#include <iostream>
#include <thread>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <vector>

#define PORT 9999
#define BUFFER_SIZE 1024



class Networking {
public:
    Networking();
    ~Networking() {
        close(my_socket);
        listen_thread.join();

    }
    void SendMessage(const std::string &message) {
        sendto(my_socket, message.c_str(), message.size(), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    }
private:
    void Listen();
private:
    int my_socket;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    std::thread listen_thread;

    // list of clients
    std::vector<struct sockaddr_in> clients;

};


#endif //GROUP3ENGINE_NETWORKING_HPP
