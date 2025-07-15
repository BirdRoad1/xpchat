#pragma once

#include <vector>
#include <xpchat/server.h>
#include <unordered_map>
#include <mutex>
class ServerList
{
private:
    ServerList() {}
    ~ServerList() {}
    std::mutex mut;
    std::unordered_map<int, Server> servers;

public:
    static ServerList &getInstance()
    {
        static ServerList instance;

        return instance;
    }
    ServerList &operator=(const ServerList &) = delete;
    ServerList(const ServerList &) = delete;

    std::vector<const Server*> getServers();
    void getServerById(int id);
    const Server *getServerBySocket(int fd);
    void addServer(Server server);
    void removeServer(int fd);
};