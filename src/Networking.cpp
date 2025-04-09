//
// Created by thomas on 01/04/25.
//

#include "Networking.hpp"

#include <string>

namespace {

void CloseSocket(int socket) {
    int status = 0;

#ifdef _WIN32
    status = shutdown(socket, SD_BOTH);
    if (status == 0) {
        status = closesocket(socket);
    }
#else
    status = shutdown(socket, SHUT_RDWR);
    if (status == 0) {
        status = close(socket);
    }
#endif
}

}

Networking::Networking()
{
    my_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (my_socket < 0)
    {
        std::cerr << "Socket creation falied";
        return;
    }
    struct timeval tv;
    tv.tv_sec = 2;  // 2 seconds timeout
    tv.tv_usec = 0;
    setsockopt(my_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);


    if(bind(my_socket, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "bind failed";
        Close();
        return;
    }
    // Set the client address to the hard coded values
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(9999);
    if (inet_pton(AF_INET, "132.145.48.206", &client_addr.sin_addr) <= 0)
    {
        std::cerr << "Invalid client address" << std::endl;
    }

    messages.reserve(100);
    listen_thread = std::thread(&Networking::Listen, this);
}

void Networking::Listen()
{
    while(running)
    {
        struct sockaddr_in this_client;
        socklen_t length = sizeof(this_client);
        int n = recvfrom(my_socket, buffer.data(), BUFFER_SIZE, 0, (struct sockaddr *)&this_client, &length);
        if (n < 0)
        {
            std::cerr << "Receive failed" << std::endl;
            continue;
        }
        buffer[n] = '\0';
        // add the data to the messages vector
        {
            std::lock_guard<std::mutex> lock(messages_mutex);
            messages.push_back(buffer);
        }
    }
}

void Networking::Close() {
    CloseSocket(my_socket);
}



std::string http_get(const std::string& url) {
    // Extract host and path from the URL
    std::string host = url;
    std::string path = "/";
    auto slashPos = url.find('/');
    if (slashPos != std::string::npos) {
        host = url.substr(0, slashPos);
        path = url.substr(slashPos);
    }

    const char* port = "80";
    std::string request =
            "GET " + path + " HTTP/1.1\r\n" +
            "Host: " + host + "\r\n" +
            "Connection: close\r\n\r\n";

    // DNS resolution
    addrinfo hints{}, *res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return "";
    }

    // Open socket
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("socket");
        freeaddrinfo(res);
        return "";
    }

    // Connect
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        CloseSocket(sock);
        freeaddrinfo(res);
        return "";
    }
    freeaddrinfo(res);

    // Send request
    if (send(sock, request.c_str(), request.length(), 0) < 0) {
        perror("send");
        CloseSocket(sock);
        return "";
    }

    // Receive and print response
    char buffer[4096];
    std::ptrdiff_t bytesRead;
    while ((bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesRead] = '\0';
    }
    // parse out the header
    std::string header(buffer);
    std::string response;
    size_t headerEnd = header.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        response = header.substr(headerEnd + 4); // Skip the header
    } else {
        std::cerr << "Invalid HTTP response" << std::endl;
        CloseSocket(sock);
        return "";
    }

    // Close socket
    CloseSocket(sock);
    return response;
}
void http_post(const std::string& url, const std::string& data)
{
    // Extract host and path from the URL
    std::string host = url;
    std::string path = "/";
    auto slashPos = url.find('/');
    if (slashPos != std::string::npos) {
        host = url.substr(0, slashPos);
        path = url.substr(slashPos);
    }

    const char* port = "80";
    std::string request =
            "POST " + path + " HTTP/1.1\r\n" +
            "Host: " + host + "\r\n" +
            "Content-Type: application/json\r\n" +
            "Content-Length: " + std::to_string(data.size()) + "\r\n" +
            "Connection: close\r\n\r\n" +
            data;

    // DNS resolution
    addrinfo hints{}, *res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return;
    }

    // Open socket
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("socket");
        freeaddrinfo(res);
        return;
    }

    // Connect
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        CloseSocket(sock);
        freeaddrinfo(res);
        return;
    }
    freeaddrinfo(res);

    // Send request
    if (send(sock, request.c_str(), request.length(), 0) < 0) {
        perror("send");
        CloseSocket(sock);
        return;
    }
    // Receive response
    char buffer[4096];
    while (recv(sock, buffer, sizeof(buffer), 0) > 0) {
    }


    // Close socket
    CloseSocket(sock);
}