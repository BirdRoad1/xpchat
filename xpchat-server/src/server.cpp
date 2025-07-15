#include "server.h"
#include <string>
#include <iostream>
#include <xpchat/chat_protocol.h>
#include <unistd.h>
#include "client_list.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

inline void handleClientConnection(std::string ip, int fd)
{
    auto &serverList = ClientList::getInstance();
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
    if (!identity.starts_with("xpchatter"))
    {
        std::cout << "Client " << identity << " is not xpchatter! Closing connection" << std::endl;
        close(fd);
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
                serverList.removePeer(fd);
                close(fd);
                return;
            }

            std::cout << "Got command: " << cmd << cmd.length() << std::endl;

            if (cmd == "DISC")
            {
                std::cout << "Client disconnected: " << ip << std::endl;
                serverList.removePeer(fd);
                close(fd);
                return;
            }
            else if (cmd == "RUSR")
            { // register user
                if (registered)
                    continue;
                if (!ChatProtocol::readString(fd, username))
                {
                    std::cout << "No username received" << std::endl;
                    continue;
                }

                serverList.addPeer({{fd, ip, identity, time(NULL)}, username});
                registered = true;

                ChatProtocol::writeString(fd, "MSG");
                ChatProtocol::writeString(fd, "Welcome to this server! I hope you enjoy your stay :)");
                ChatProtocol::flush(fd);
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

// Connect to central server
bool connectToCentral()
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("192.168.100.1");
    addr.sin_port = htons(28492);

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
    ChatProtocol::writeString(sock, "RSRV");
    ChatProtocol::flush(sock);
    return true;
}

int mainServer()
{
    std::cout << "xpchatserver SERVER" << std::endl;

    std::string warning = "Your server will not be advertised to clients but they will be able to connect directly.";
    try
    {
        if (!connectToCentral())
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
