#include "chat.h"
#include "winsock2.h"
#include <iostream>
#include <windows.h>
#include <limits>
#include <xpchat/chat_protocol.h>
#include <queue>

bool Chat::disconnect()
{
    if (fd < 0)
        return false;

    ChatProtocol::writeString(fd, "DISC");
    ChatProtocol::flush(fd);
    bool result = closesocket(fd) == 0;
    fd = -1;

    return result;
}

bool Chat::connect(const Server server)
{
    if (fd != -1)
    {
        disconnect();
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(server.ip.c_str());
    serv_addr.sin_port = htons(28592);

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

    std::cout << "Connected to server!" << std::endl;
    ChatProtocol::writeString(sock, "xpchatter v1.0");
    
    ChatProtocol::writeString(sock, "RUSR");
    ChatProtocol::writeString(sock, "username");
    ChatProtocol::flush(sock);

    fd = sock;
    this->server = server;
    return true;
}

const Server* Chat::getActiveServer() {
    if (fd == -1) return nullptr;

    return &this->server;
}