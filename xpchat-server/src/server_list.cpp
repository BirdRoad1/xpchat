#include "server_list.h"

std::vector<const Server *> ServerList::getServers()
{
    std::lock_guard lock(mut);

    std::vector<const Server *> vec;
    vec.reserve(servers.size());
    for (const auto &kv : servers)
    {
        vec.push_back(&kv.second);
    }

    return vec;
}

const Server *ServerList::getServerBySocket(int fd)
{
    std::lock_guard lock(mut);

    auto it = servers.find(fd);
    if (it == servers.end())
    {
        return nullptr;
    }

    return &(it->second);
}

void ServerList::removeServer(int fd)
{
    std::lock_guard lock(mut);
    servers.erase(fd);
}

void ServerList::addServer(Server server)
{
    std::lock_guard lock(mut);
    servers.erase(server.socket);

    servers.insert({server.socket, server});
}