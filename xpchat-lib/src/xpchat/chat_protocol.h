#pragma once

#include "socket.h"
#include "server.h"
#include "client.h"
#include <queue>
#include <string>
#include <mutex>
#include <memory>
#include <unordered_map>

#ifdef _WIN32
typedef SOCKET socket_t;
#else
typedef int socket_t;
#endif

struct Chunk
{
    std::unique_ptr<char[]> ptr;
    size_t length;
};

class ChatProtocol
{
private:
    static std::unordered_map<socket_t, std::recursive_mutex> sendQueueMutexes;
    static std::unordered_map<int, std::queue<Chunk>> sendQueueMap;

    static std::recursive_mutex &getMutex(int fd);
    static std::queue<Chunk> &getSendQueue(int fd);
    static bool enqueueBytes(int fd, const void *ptr, size_t size);
    static std::unique_ptr<Chunk> dequeueChunk(int fd);

public:
    static bool writeInt(socket_t fd, const int data);
    static bool readInt(socket_t fd, int &data);
    static bool writeServer(socket_t fd, const Server &server);
    static bool readServer(socket_t fd, Server &server);
    static bool writeString(socket_t fd, const std::string &str);
    static bool readString(socket_t fd, std::string &str);
    static bool writeClient(socket_t fd, const Client &client);
    static bool readClient(socket_t fd, Client &client);
    static bool flush(socket_t fd);
};