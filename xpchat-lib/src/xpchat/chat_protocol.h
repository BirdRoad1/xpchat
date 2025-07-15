#pragma once

#include "socket.h"
#include "server.h"
#include <string>
#ifdef _WIN32
#endif

#ifdef _WIN32
typedef SOCKET socket_t;
#else
typedef int socket_t;
#endif

class ChatProtocol
{
public:
    static bool writeInt(socket_t fd, const int data);
    static bool readInt(socket_t fd, int &data);
    static bool writeServer(socket_t fd, const Server &server);
    static bool readServer(socket_t fd, Server &server);
    static bool writeString(socket_t fd, const std::string &str);
    static bool readString(socket_t fd, std::string &str);
};