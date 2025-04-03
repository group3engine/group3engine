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

#define PORT 9998
#define BUFFER_SIZE 1024



class Networking {
public:
    Networking();
    ~Networking() {
        close(my_socket);
        if (listen_thread.joinable()) listen_thread.detach();
    }
    void SendMessage(const std::string &message) {
        sendto(my_socket, message.c_str(), message.size(), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    }
    std::vector<std::array<char, BUFFER_SIZE>> GetMessages() {
        // copy the messages to a new vector and return it
        // and clear the original vector
        std::vector<std::array<char, BUFFER_SIZE>> temp = messages;
        messages.clear();
        return temp;
    }
private:
    [[noreturn]] void Listen();
private:
    int my_socket;
    struct sockaddr_in server_addr, client_addr;
    std::array<char, BUFFER_SIZE> buffer;
    std::vector<std::array<char, BUFFER_SIZE>> messages;
    std::thread listen_thread;
    bool running = true;

    // list of clients
    std::vector<struct sockaddr_in> clients;

};


#endif //GROUP3ENGINE_NETWORKING_HPP
