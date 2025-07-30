#include "central.h"

#include <string>
#include <iostream>
#include <xpchat/chat_protocol.h>
#include <unistd.h>
#include "server_list.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

inline void closeServerConnection(int fd, std::string ip)
{
    auto &serverList = ServerList::getInstance();
    Server server;

    if (serverList.getPeerById(fd, server))
    {
        std::cout << "Client disconnected: " << ip << ", software: " << server.identifier << std::endl;
    }
    else
    {
        std::cout << "Unknown client disconnected: " << ip << std::endl;
    }

    serverList.removePeer(fd);
    close(fd);
}

inline void handleServerConnection(std::string ip, int fd)
{
    auto &serverList = ServerList::getInstance();
    std::cout << "Received connection from " << ip << std::endl;

    // Let it identify itself first
    std::string identity;
    if (!ChatProtocol::readString(fd, identity))
    {
        std::cout << "Client did not identify! Closing connection" << std::endl;
        closeServerConnection(fd, ip);
        return;
    }

    std::cout << "Identity: " << identity << std::endl;
    bool isServer = identity.starts_with("xpchatter-server");

    std::string publicIP;
    if (isServer)
    {
        if (!ChatProtocol::readString(fd, publicIP))
        {
            std::cout << "Client did not tell us their public IP! Closing connection" << std::endl;
            closeServerConnection(fd, ip);
            return;
        }

        std::cout << "Public IP: " << publicIP << std::endl;
    }

    time_t lastRegistrationTime = 0;
    while (1)
    {
        try
        {
            std::string cmd;
            if (!ChatProtocol::readString(fd, cmd))
            {
                break;
            }

            std::cout << "Got command: " << cmd << cmd.length() << std::endl;

            if (cmd == "RSRV" && isServer)
            {
                time_t now = time(NULL);
                if (now - lastRegistrationTime <= 30)
                {
                    // too soon
                    std::cout << "Warning: client " << ip << " tried re-registering too quickly! (" << now - lastRegistrationTime << " seconds)" << std::endl;
                }
                else
                {
                    // Register server
                    lastRegistrationTime = now;
                    serverList.addPeer({{fd, publicIP, identity, lastRegistrationTime}});
                }
            }
            else if (cmd == "LSRV")
            {
                // List servers
                std::vector<const Server *> servers = serverList.getPeers();
                ChatProtocol::writeString(fd, "LSRV");
                ChatProtocol::flush(fd);
                for (auto *server : servers)
                {
                    ChatProtocol::writeString(fd, "SSRV");
                    ChatProtocol::writeServer(fd, *server);
                    ChatProtocol::flush(fd);
                }

                ChatProtocol::writeString(fd, "NSRV");
                ChatProtocol::flush(fd);
            }
            else if (cmd == "EJSRV")
            { // event join server
                int id;
                if (!ChatProtocol::readInt(fd, id))
                {
                    std::cout << "Failed to read server id" << std::endl;
                    continue;
                }
                Server server;
                serverList.getPeerById(id, server);
                std::cout << "Client: " << ip << " joined server: " << server.ip << std::endl;
            }
            else
            {
                std::cout << "Unknown message: " << cmd << std::endl;
            }
        }
        catch (std::exception &ex)
        {
            std::cout << "Caught exception in handle client thread" << ex.what() << std::endl;
            break;
        }
    }

    closeServerConnection(fd, ip);
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
        std::thread(handleServerConnection, ip, clnt_sock).detach();
    }
    return 0;
}
