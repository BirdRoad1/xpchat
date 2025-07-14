#include "central.h"
#include "chat_protocol.h"
#include <limits>
#include <stdexcept>
#include <iostream>

int Central::fd = -1;

bool Central::connect()
{
    if (fd != -1)
    {
        return true;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("192.168.100.1");
    serv_addr.sin_port = htons(28492);

    // create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sizeof(serv_addr) > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        throw std::overflow_error("Structure size exceeds int capacity");
    }

    // connect
    if (::connect(sock, (sockaddr *)&serv_addr, static_cast<int>(sizeof(serv_addr))))
    {
        WCHAR buffer[100];
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), 0, buffer, 100, NULL);
        std::wcout << L"Failed to connect, error: " << buffer << std::endl;
        closesocket(sock);
        return false;
    }

    std::cout << "Connected to central!" << std::endl;
    std::string identity("xpchatter v1.0");
    size_t idLen = identity.length();
    send(sock, reinterpret_cast<const char *>(&idLen), sizeof(idLen), 0);
    send(sock, identity.c_str(), idLen, 0);

    fd = sock;
    return true;
}

void Central::listServers()
{
    if (fd < 0)
    {
        return;
    }

    if (!ChatProtocol::writeString(fd, "LSRV"))
    {
        std::cout << "Failed to list servers" << std::endl;
        return;
    }

    std::cout << "TIME TO LIST " << std::endl;

    std::string cmd;

    do
    {
        ChatProtocol::readString(fd, cmd);
        std::cout << "CMD:" << cmd << std::endl;

        if (cmd == "SSRV") // start server
        {
            Server server;
            ChatProtocol::readServer(fd, server);
            std::cout << "GET SERVER:" << std::endl;
            std::cout << "IP:" << server.ip << std::endl;
            std::cout << "ID:" << server.identifier << std::endl;
            std::cout << "TS:" << server.connectedTimestamp << std::endl;
            std::cout << "END SERVER:" << std::endl;
        }
    } while (cmd != "NSRV"); // end servers
}