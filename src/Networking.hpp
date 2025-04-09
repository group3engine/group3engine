//
// Created by thomas on 01/04/25.
//

#ifndef GROUP3ENGINE_NETWORKING_HPP
#define GROUP3ENGINE_NETWORKING_HPP

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif

    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    /* Assume that any non-Windows platform uses POSIX-style sockets instead. */
    #include <arpa/inet.h>
    #include <netdb.h> /* Needed for getaddrinfo() and freeaddrinfo() */
    #include <sys/socket.h>
    #include <unistd.h> /* Needed for close() */
#endif

#include <iostream>
#include <thread>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <array>
#include <vector>
#include <mutex>


#define PORT 9998
#define BUFFER_SIZE 1024



class Networking {
public:
    Networking();
    ~Networking() {
        running = false;
        listen_thread.join();

        Close();
    }
    void SendMessage(const std::string &message) {
        sendto(my_socket, message.c_str(), message.size(), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    }
    std::vector<std::array<char, BUFFER_SIZE>> GetMessages() {
        // copy the messages to a new vector and return it
        // and clear the original vector
        std::vector<std::array<char, BUFFER_SIZE>> temp;
        {
            std::lock_guard<std::mutex> lock(messages_mutex);
            temp = messages;
            messages.clear();
        }
        return temp;
    }
private:
    void Close();

    void Listen();
private:
    int my_socket;
    struct sockaddr_in server_addr, client_addr;
    std::array<char, BUFFER_SIZE> buffer;
    std::vector<std::array<char, BUFFER_SIZE>> messages;
    std::thread listen_thread;
    bool running = true;

    // list of clients
    std::vector<struct sockaddr_in> clients;

    std::mutex messages_mutex;


};

std::string http_get(const std::string& url);
void http_post(const std::string& url, const std::string& data);
#endif //GROUP3ENGINE_NETWORKING_HPP
