#include "chat_protocol.h"

#ifdef __linux__
#include <netinet/in.h>
#include <arpa/inet.h>
#elif _WIN32
#include <winsock2.h>
#else
#error OS not supported
#endif

#include "iostream"

bool ChatProtocol::writeString(socket_t fd, const std::string &str)
{
    size_t ipLen = str.size();
    if (send(fd, reinterpret_cast<const char *>(&ipLen), sizeof(size_t), 0) < sizeof(size_t))
        return false;
    if (send(fd, &str[0], str.length(), 0) < str.length())
        return false;

    return true;
}

bool ChatProtocol::writeServer(socket_t fd, const Server &server)
{
    if (!writeString(fd, "SSRV"))
        return false;
    // Send IP
    if (!writeString(fd, server.ip))
        return false;

    // Send ID
    if (!writeString(fd, server.identifier))
        return false;

    // Send timestamp
    if (send(fd, reinterpret_cast<const char *>(&server.connectedTimestamp), sizeof(server.connectedTimestamp), 0) < sizeof(server.connectedTimestamp))
        return false;
}

bool ChatProtocol::readString(socket_t fd, std::string &str)
{
    size_t len = 0;
    if (recv(fd, reinterpret_cast<char *>(&len), sizeof(len), MSG_WAITALL) < sizeof(len))
        return false;

    if (len >= 1024)
    {
        throw std::runtime_error("Invalid string of length " + std::to_string(len) + " received (max 1024)");
    }

    std::string buffer(len, '\0');
    if (recv(fd, reinterpret_cast<char *>(&buffer[0]), len, 0) < len)
        return false;

    str = buffer;

    return true;
}

bool ChatProtocol::readServer(socket_t fd, Server &server)
{
    std::string ip;
    if (!readString(fd, ip))
        return false;

    std::string id;
    if (!readString(fd, id))
        return false;

    time_t connectedTimestamp;
    if (recv(fd, reinterpret_cast<char *>(&connectedTimestamp), sizeof(time_t), 0) < sizeof(time_t))
        return false;

    server = {.ip = ip, .identifier = id, .connectedTimestamp = connectedTimestamp};
    return true;
}