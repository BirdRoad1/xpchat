#pragma once

#include <vector>
#include <xpchat/server.h>
#include <unordered_map>
#include <mutex>
#include <xpchat/peer.h>

template <typename T>
class PeerList
{
protected:
    PeerList() {}
    ~PeerList() {}

private:
    std::mutex mut;
    std::unordered_map<int, T> servers;

public:
    static PeerList &getInstance()
    {
        static PeerList instance;

        return instance;
    }
    PeerList &operator=(const PeerList &) = delete;
    PeerList(const PeerList &) = delete;

    std::vector<const T *> getPeers()
    {
        std::lock_guard lock(mut);

        std::vector<const T *> vec;
        vec.reserve(servers.size());
        for (const auto &kv : servers)
        {
            vec.push_back(&kv.second);
        }

        return vec;
    }

    const T *getPeerBySocket(int fd)
    {
        std::lock_guard lock(mut);

        auto it = servers.find(fd);
        if (it == servers.end())
        {
            return nullptr;
        }

        return &(it->second);
    }

    void removePeer(int fd)
    {
        std::lock_guard lock(mut);
        servers.erase(fd);
    }

    void addPeer(T server)
    {
        std::lock_guard lock(mut);
        servers.erase(server.socket);

        servers.insert({server.socket, server});
    }

    bool getPeerById(int id, T &out)
    {
        std::lock_guard lock(mut);
        for (auto &kv : servers)
        {
            if (kv.second.socket == id)
            {
                out = kv.second;
                return true;
            }
        }

        return false;
    }
};