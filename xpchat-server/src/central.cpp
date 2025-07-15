#include "central.h"

#include <string>
#include <iostream>
#include <xpchat/chat_protocol.h>
#include <unistd.h>
#include "server_list.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

inline void handleServerConnection(std::string ip, int fd)
{
    auto &serverList = ServerList::getInstance();
    std::cout << "Received connection from " << ip << std::endl;

    // Let it identify itself first
    std::string identity;
    if (!ChatProtocol::readString(fd, identity))
    {
        std::cout << "Client did not identify! Closing connection" << std::endl;
        close(fd);
        return;
    }

    std::cout << "Identity: " << identity << std::endl;

    bool isServer = identity.starts_with("xpchatter-server");

    time_t lastRegistrationTime = 0;
    while (1)
    {
        try
        {
            std::string cmd;
            if (!ChatProtocol::readString(fd, cmd))
            {
                std::cout << "Failed to read command! Closing connection" << std::endl;
                serverList.removePeer(fd);
                close(fd);
                return;
            }

            std::cout << "Got command: " << cmd << cmd.length() << std::endl;

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
                    serverList.addPeer({{fd, ip, identity, lastRegistrationTime}});
                }
            }
            else if (cmd == "LSRV")
            {
                // List servers
                std::vector<const Server *> servers = serverList.getPeers();
                // size_t serversCount = servers.size();
                std::cout << "SENDING LSRV" << std::endl;
                ChatProtocol::writeString(fd, "LSRV");
                ChatProtocol::flush(fd);
                for (auto *server : servers)
                {
                    std::cout << "SENDING SERVER" << std::endl;
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
                std::cout << "NO match" << std::endl;
            }
        }
        catch (std::exception &ex)
        {
            std::cout << "Caught exception in handle client thread" << ex.what() << std::endl;
            close(fd);
            serverList.removePeer(fd);
        }
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
        std::thread(handleServerConnection, ip, clnt_sock).detach();
    }
    return 0;
}
