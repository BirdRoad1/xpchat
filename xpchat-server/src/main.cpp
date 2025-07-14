#include <iostream>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <unistd.h>
#include "server_list.h"
#include <chat_protocol.h>

void handleClient(std::string ip, int fd)
{
    std::cout << "Received connection from " << ip << std::endl;

    // Let it identify itself first
    std::string identityBuf;
    if (!ChatProtocol::readString(fd, identityBuf)) {
        std::cout << "Client did not identify! Closing connection" << std::endl;
        close(fd);
        return;
    }

    // read(fd, reinterpret_cast<char *>(&idLen), sizeof(idLen));
    // std::string identityBuf(idLen + 1, '\0');
    // ssize_t bytesRead;
    // if ((bytesRead = read(fd, identityBuf.data(), idLen)) < idLen)
    // {
    //     std::cout << "Client did not identify! Closing connection" << std::endl;
    //     close(fd);
    //     return;
    // }
    // identityBuf.resize(bytesRead);


    std::cout << "Identity: " << identityBuf << std::endl;

    bool isServer = identityBuf.starts_with("xpchatter-server");

    std::cout << "IS_SERVER: " << isServer << std::endl;

    time_t lastRegistrationTime = 0;
    while (1)
    {
        std::string cmd;
        if (!ChatProtocol::readString(fd, cmd)) {
            std::cout << "Failed to read command! Closing connection" << std::endl;
            close(fd);
            return;
        }

        // size_t cmdLength = 4;
        // std::string cmd(cmdLength + 1, '\0');
        // if (read(fd, cmd.data(), cmdLength) != cmdLength)
        // {
        //     std::cout << "Failed to read command! Closing connection" << std::endl;
        //     close(fd);
        //     return;
        // }
        // cmd.resize(cmdLength);


        if (cmd == "RSRV" && isServer)
        {
            time_t now = time(NULL);
            if (now - lastRegistrationTime <= 30)
            {
                // too soon
                std::cout << "Warning: client " << ip << " tried registering too quickly! (" << now - lastRegistrationTime << " seconds)" << std::endl;
            }
            else
            {
                // Register server
                lastRegistrationTime = now;
                std::cout << "ADD SERVER132:" << identityBuf << std::endl;
                ServerList::getInstance().addServer({.socket = fd, .ip = ip, .identifier = identityBuf, .connectedTimestamp = lastRegistrationTime});
            }
        }
        else if (cmd == "LSRV")
        {
            // List servers
            std::vector<const Server *> servers = ServerList::getInstance().getServers();
            // size_t serversCount = servers.size();
            std::cout << "SENDING LSRV" << std::endl;
            ChatProtocol::writeString(fd, "LSRV");
            for (auto *server : servers)
            {
                ChatProtocol::writeServer(fd, *server);
            }

            ChatProtocol::writeString(fd, "NSRV");
        }
        else
        {
            std::cout << "NO match" << std::endl;
        }

        std::cout << "Got command: " << cmd << std::endl;
    }
}

int mainCentral()
{
    // Central server
    std::cout << "xpchatserver CENTRAL" << std::endl;
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    serv_addr.sin_port = htons(28492);
    int reuse = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
    {
        std::cout << "Failed to set reuseaddr" << std::endl;
    }

    bind(serv_sock, (sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(serv_sock, 20);
    std::cout << "Listening for connections..." << std::endl;
    while (1)
    {
        sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);

        int clnt_sock = accept(serv_sock, (sockaddr *)&client_addr, &client_addr_size);
        if (clnt_sock == -1)
        {
            std::cout << "accept() failed" << std::endl;
            continue;
        }

        std::string ip(inet_ntoa(client_addr.sin_addr));
        std::thread(handleClient, ip, clnt_sock).detach();
    }
    return 0;
}

int mainServer()
{
    std::cout << "xpchatserver SERVER" << std::endl;
    // Connect to central server
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(28492);

    // create socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sizeof(addr) > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        throw std::overflow_error("Structure size exceeds int capacity");
    }

    // connect
    if (::connect(sock, (sockaddr *)&addr, static_cast<int>(sizeof(addr))))
    {
        std::cout << "Failed to connect to central" << std::endl;
        return 0;
    }

    std::cout << "Connected to central!" << std::endl;

    std::string identity("xpchatter-server v1.0");
    ChatProtocol::writeString(sock, identity);
    // size_t idLen = identity.size();
    // send(sock, reinterpret_cast<char*>(&idLen), sizeof(idLen), 0);
    // send(sock, identity.c_str(), identity.length(), 0);
    // send(sock, "RSRV", 4, 0); // register server
    ChatProtocol::writeString(sock, "RSRV");

    // Allow clients to connect!

    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    serv_addr.sin_port = htons(28492);
    int reuse = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
    {
        std::cout << "Failed to set reuseaddr" << std::endl;
    }

    bind(serv_sock, (sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(serv_sock, 20);
    std::cout << "Listening for connections..." << std::endl;
    while (1)
    {
        sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);

        int clnt_sock = accept(serv_sock, (sockaddr *)&client_addr, &client_addr_size);
        if (clnt_sock == -1)
        {
            std::cout << "accept() failed" << std::endl;
            continue;
        }

        std::string ip(inet_ntoa(client_addr.sin_addr));
        std::thread(handleClient, ip, clnt_sock).detach(); //TODO: change func
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        std::string type(argv[1]);
        std::transform(type.begin(), type.end(), type.begin(),
                       [](unsigned char c)
                       { return std::tolower(c); });

        if (type == "central")
        {
            return mainCentral();
        }
    }

    return mainServer();
}