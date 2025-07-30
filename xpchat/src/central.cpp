#include "central.h"
#include "winsock2.h"
#include <xpchat/chat_protocol.h>
#include <limits>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <ws2tcpip.h>

int Central::fd = -1;

bool Central::connect()
{
    if (fd != -1)
    {
        return true;
    }

    addrinfo hints;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_family = AF_INET;

    addrinfo *result;

    if (getaddrinfo("xpchatcentral.jlmsz.com", NULL, &hints, &result) != 0)
    {
        std::cout << "Failed to resolve xpchatcentral.jlmsz.com!" << std::endl;
        return false;
    }

    if (result == NULL)
    {
        std::cout << "No IPs found for xpchatcentral.jlmsz.com" << std::endl;
        return false;
    }

    sockaddr_in *resolvedAddr = (sockaddr_in *)result->ai_addr;

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = resolvedAddr->sin_addr.s_addr;
    serv_addr.sin_port = htons(28492);

    freeaddrinfo(result);

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
    ChatProtocol::writeString(sock, "xpchatter v1.0");
    ChatProtocol::flush(sock);

    fd = sock;
    return true;
}

bool Central::disconnect()
{
    if (fd < 0)
        return false;

    ChatProtocol::flush(fd);
    closesocket(fd);
    fd = -1;
    return true;
}

bool Central::isConnected()
{
    return fd >= 0;
}

bool Central::listServers(std::vector<Server> &servers)
{
    if (fd < 0)
    {
        return false;
    }

    std::cout << "LIST SERV" << std::endl;

    std::vector<Server> srv;

    if (!ChatProtocol::writeString(fd, "LSRV"))
    {
        return false;
    }

    ChatProtocol::flush(fd);

    std::string cmd;

    std::cout << "LIST SERV2" << std::endl;
    do
    {
        if (!ChatProtocol::readString(fd, cmd))
        {
            return false;
        }

        std::cout << "Reading str: " << cmd << std::endl;

        if (cmd == "SSRV") // start server
        {
            std::cout << "Found SSRV" << std::endl;
            Server server;
            if (!ChatProtocol::readServer(fd, server))
            {
                std::cout << "Failed to read server" << std::endl;
                return false;
            }

            std::cout << "Push server" << std::endl;
            srv.push_back(server);
        }
    } while (cmd != "NSRV"); // end servers
    servers = srv;
    std::cout << "LIST SERV3" << std::endl;

    return true;
}

bool Central::sendServerJoin(int id)
{
    if (fd < 0)
        return false;

    std::vector<Server> srv;

    if (!ChatProtocol::writeString(fd, "EJSRV")) // event join server
        return false;

    if (!ChatProtocol::writeInt(fd, id))
        return false;

    ChatProtocol::flush(fd);

    return true;
}