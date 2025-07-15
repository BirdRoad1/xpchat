#include "xpchat/chat_protocol.h"

#ifdef __linux__
#include <netinet/in.h>
#include <arpa/inet.h>
#elif _WIN32
#include <winsock2.h>
#else
#error OS not supported
#endif

#include "iostream"
#include <cstring>
#include <memory>
#include <unordered_map>

std::unordered_map<int, std::queue<Chunk>> ChatProtocol::sendQueueMap;
std::unordered_map<socket_t, std::recursive_mutex> ChatProtocol::sendQueueMutexes;

std::queue<Chunk> &ChatProtocol::getSendQueue(int fd)
{
    return ChatProtocol::sendQueueMap.try_emplace(fd).first->second;
}

std::recursive_mutex &ChatProtocol::getMutex(int fd)
{
    return ChatProtocol::sendQueueMutexes.try_emplace(fd).first->second;
}

bool ChatProtocol::enqueueBytes(int fd, const void *ptr, size_t size)
{
    std::lock_guard lock(getMutex(fd));

    try
    {
        std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);

        std::cout << "size:" << size << std::endl;
        std::memcpy(buffer.get(), ptr, size);
        auto &queue = getSendQueue(fd);
        queue.push(Chunk{std::move(buffer), size});
        return true;
    }
    catch (std::bad_alloc &ex)
    {
        return false;
    }
}

std::unique_ptr<Chunk> ChatProtocol::dequeueChunk(int fd)
{
    std::lock_guard lock(getMutex(fd));
    auto &queue = getSendQueue(fd);

    if (queue.empty())
        return nullptr;

    auto up = std::make_unique<Chunk>(std::move(queue.front()));
    queue.pop();
    return up;
}

bool ChatProtocol::writeInt(socket_t fd, const int data)
{
    return ChatProtocol::enqueueBytes(fd, &data, sizeof(data));
}

bool ChatProtocol::readInt(socket_t fd, int &data)
{
    std::lock_guard lock(getMutex(fd));

    int out;
    if (recv(fd, reinterpret_cast<char *>(&out), sizeof(out), 0) < sizeof(out))
        return false;

    data = out;
    return true;
}

bool ChatProtocol::writeString(socket_t fd, const std::string &str)
{
    std::lock_guard lock(getMutex(fd));
    std::cout << "write string" << std::endl;
    std::cout << "after mutex" << std::endl;

    size_t ipLen = str.length();
    if (!ChatProtocol::enqueueBytes(fd, &ipLen, sizeof(ipLen)))
    {
        std::cout << "Failed to enqueue bytes for len" << std::endl;
        return false;
    }

    std::cout << "After first enqueu" << std::endl;

    if (ipLen > 0)
    {
        std::cout << "string empty" << std::endl;
        if (!ChatProtocol::enqueueBytes(fd, &str[0], ipLen))
        {
            std::cout << "Failed to enqueue bytes for str" << std::endl;
            return false;
        }
        std::cout << "After 2nd enqueu" << std::endl;
    }

    std::cout << "Finished writing" << std::endl;
    return true;
}

bool ChatProtocol::writeServer(socket_t fd, const Server &server)
{
    std::lock_guard lock(getMutex(fd));
    if (!writeInt(fd, server.socket))
        return false;
    // Send IP
    if (!writeString(fd, server.ip))
        return false;

    // Send ID
    if (!writeString(fd, server.identifier))
        return false;

    // Send timestamp
    if (!enqueueBytes(fd, &server.connectedTimestamp, sizeof(server.connectedTimestamp)))
        return false;

    return true;
}

bool ChatProtocol::readString(socket_t fd, std::string &str)
{
    std::lock_guard lock(getMutex(fd));

    size_t len = 0;
    if (recv(fd, reinterpret_cast<char *>(&len), sizeof(len), MSG_WAITALL) < sizeof(len))
        return false;

    if (len >= 1024)
    {
        throw std::runtime_error("Invalid string of length " + std::to_string(len) + " received (max 1024)");
    }

    std::string buffer(len, '\0');
    if (recv(fd, reinterpret_cast<char *>(&buffer[0]), len, 0) < len)
        return false;

    str = buffer;

    return true;
}

bool ChatProtocol::readServer(socket_t fd, Server &server)
{
    std::lock_guard lock(getMutex(fd));

    int socket = -1;
    if (!readInt(fd, socket))
        return false;

    std::string ip;
    if (!readString(fd, ip))
        return false;

    std::string id;
    if (!readString(fd, id))
        return false;

    time_t connectedTimestamp;
    if (recv(fd, reinterpret_cast<char *>(&connectedTimestamp), sizeof(time_t), 0) < sizeof(time_t))
        return false;

    server = {
        {socket,
         ip,
         id,
         connectedTimestamp}};
    return true;
}

bool ChatProtocol::writeClient(socket_t fd, const Client &client)
{
    std::lock_guard lock(getMutex(fd));

    if (!writeInt(fd, client.socket))
        return false;
    // Send IP
    if (!writeString(fd, client.ip))
        return false;

    // Send ID
    if (!writeString(fd, client.identifier))
        return false;

    // Send username
    if (!writeString(fd, client.username))
        return false;

    // Send timestamp
    if (!enqueueBytes(fd, &client.connectedTimestamp, sizeof(client.connectedTimestamp)))
        return false;

    return true;
}

bool ChatProtocol::readClient(socket_t fd, Client &client)
{
    std::lock_guard lock(getMutex(fd));

    int socket = -1;
    if (!readInt(fd, socket))
        return false;

    std::string ip;
    if (!readString(fd, ip))
        return false;

    std::string id;
    if (!readString(fd, id))
        return false;

    std::string username;
    if (!readString(fd, username))
        return false;

    time_t connectedTimestamp;
    if (recv(fd, reinterpret_cast<char *>(&connectedTimestamp), sizeof(time_t), 0) < sizeof(time_t))
        return false;

    client = {
        {socket,
         ip,
         id,
         connectedTimestamp},
        username,
    };
    return true;
}

bool ChatProtocol::flush(socket_t fd)
{
    std::lock_guard lock(getMutex(fd));
    auto &queue = ChatProtocol::getSendQueue(fd);
    
    std::cout << "Trying to flush " << queue.size() << " chunks to " << fd << "!" << std::endl;
    while (!queue.empty())
    {
        auto chunkPtr = ChatProtocol::dequeueChunk(fd);
        if (!chunkPtr)
            continue;

        const char *data = chunkPtr->ptr.get();
        if (!data)
            continue;

        if (send(fd, data, chunkPtr->length, 0) < 0)
        {
            std::cout << "Failed to send " << chunkPtr->length << " bytes!" << std::endl;
            return false;
        }

        std::cout << "Sent " << chunkPtr->length << " bytes!" << std::endl;
    }

    return true;
}