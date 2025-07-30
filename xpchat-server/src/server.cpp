#include "server.h"
#include <string>
#include <iostream>
#include <xpchat/chat_protocol.h>
#include <unistd.h>
#include "client_list.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

inline void closeClientConnection(int fd, std::string ip)
{
    auto &clientList = ClientList::getInstance();
    Client client;

    if (clientList.getPeerById(fd, client))
    {
        std::cout << "Client disconnected: " << ip << ", software: " << client.identifier << std::endl;
    }
    else
    {
        std::cout << "Unknown client disconnected: " << ip << std::endl;
    }

    clientList.removePeer(fd);
    close(fd);
}

inline void handleClientConnection(std::string ip, int fd)
{
    auto &clientList = ClientList::getInstance();
    std::cout << "Received connection from " << ip << std::endl;

    // Let it identify itself first
    std::string identity;
    if (!ChatProtocol::readString(fd, identity))
    {
        std::cout << "Client did not identify! Closing connection" << std::endl;
        closeClientConnection(fd, ip);
        return;
    }

    std::cout << "Identity: " << identity << std::endl;
    if (!identity.starts_with("xpchatter"))
    {
        std::cout << "Client " << identity << " is not xpchatter! Closing connection" << std::endl;
        closeClientConnection(fd, ip);
        return;
    }

    bool registered = false;
    std::string username;

    while (1)
    {
        try
        {
            std::string cmd;
            if (!ChatProtocol::readString(fd, cmd))
            {
                std::cout << "Failed to read command! Closing connection" << std::endl;
                break;
            }

            std::cout << "Got command: " << cmd << cmd.length() << std::endl;

            if (cmd == "DISC")
            {
                std::cout << "Client disconnected: " << ip << std::endl;
                break;
            }
            else if (cmd == "RUSR")
            { // register user
                if (registered)
                {
                    std::cout << "Already registered" << std::endl;
                    ChatProtocol::writeString(fd, "RUSR");
                    ChatProtocol::writeInt(fd, 0);
                    ChatProtocol::writeString(fd, "You are already registered!");
                }

                if (!ChatProtocol::readString(fd, username))
                {
                    std::cout << "No username received" << std::endl;
                    continue;
                }

                bool duplicate = false;
                for (auto &peer : clientList.getPeers())
                {
                    if (peer->username == username)
                    {
                        duplicate = true;
                    }
                }

                if (duplicate)
                {
                    std::cout << "DUPE" << std::endl;
                    ChatProtocol::writeString(fd, "RUSR");
                    ChatProtocol::writeInt(fd, 0);
                    ChatProtocol::writeString(fd, "The username is already in use");
                    ChatProtocol::flush(fd);
                    continue;
                }
                else
                {
                    ChatProtocol::writeString(fd, "RUSR");
                    ChatProtocol::writeInt(fd, 1);
                    ChatProtocol::flush(fd);
                }

                clientList.addPeer({{fd, ip, identity, time(NULL)}, username});
                registered = true;

                ChatProtocol::writeString(fd, "MSG");
                ChatProtocol::writeString(fd, "Welcome to this server! I hope you enjoy your stay :)");
                ChatProtocol::flush(fd);
            }
            else if (cmd == "MSG")
            {
                if (!registered)
                    continue;
                std::string msg;
                if (!ChatProtocol::readString(fd, msg))
                {
                    std::cout << "Failed to get msg" << std::endl;
                    continue;
                }

                std::cout << "[" << username << "] " << msg << std::endl;
                auto clients = clientList.getPeers();
                for (auto &client : clients)
                {
                    ChatProtocol::writeString(client->socket, "MSG");
                    ChatProtocol::writeString(client->socket, username + ": " + msg);
                    if (!ChatProtocol::flush(client->socket))
                    {
                        break;
                    }
                }
            }
        }
        catch (std::exception &ex)
        {
            std::cout << "Caught exception in handle client thread" << ex.what() << std::endl;
            break;
        }
    }

    closeClientConnection(fd, ip);
}

// Connect to central server
bool connectToCentral(std::string publicIP)
{
    addrinfo hints;
    hints.ai_flags = AI_ADDRCONFIG;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_family = AF_INET;
    addrinfo *result;
    int addrResult = getaddrinfo("xpchatcentral.jlmsz.com", NULL, &hints, &result);
    if (addrResult != 0)
    {
        std::cout << "Failed to resolve xpchatcentral.jlmsz.com! Error code: " << addrResult << std::endl;
        return false;
    }

    if (result == NULL)
    {
        std::cout << "No IPs found for xpchatcentral.jlmsz.com" << std::endl;
        return false;
    }

    sockaddr_in *resolvedAddr = (sockaddr_in *)result->ai_addr;
    std::cout << "Resolved central IP: " << inet_ntoa(resolvedAddr->sin_addr) << std::endl;

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = resolvedAddr->sin_addr.s_addr;
    addr.sin_port = htons(28492);

    freeaddrinfo(result);

    // create socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sizeof(addr) > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        throw std::overflow_error("Structure size exceeds int capacity");
    }

    // connect
    if (::connect(sock, (sockaddr *)&addr, static_cast<int>(sizeof(addr))) < 0)
    {
        perror("Error connecting to central");
        return false;
    }

    std::string identity("xpchatter-server v1.0");
    ChatProtocol::writeString(sock, identity);
    ChatProtocol::writeString(sock, publicIP);
    ChatProtocol::writeString(sock, "RSRV");
    ChatProtocol::flush(sock);
    return true;
}

int mainServer(std::string publicIP)
{
    std::cout << "xpchatserver SERVER" << std::endl;

    std::string warning = "Your server will not be advertised to clients but they will be able to connect directly.";
    try
    {
        if (!connectToCentral(publicIP))
        {
            std::cout << "Failed to connect to central. " << warning << std::endl;
        }
        else
        {
            std::cout << "Connected to central!" << std::endl;
        }
    }
    catch (std::exception &ex)
    {
        std::cout << "Exception while connecting to central: " << ex.what() << ". " << warning << std::endl;
    }

    // Allow clients to connect!

    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    serv_addr.sin_port = htons(28592);
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
        std::thread(handleClientConnection, ip, clnt_sock).detach(); // TODO: change func
    }

    return 0;
}
