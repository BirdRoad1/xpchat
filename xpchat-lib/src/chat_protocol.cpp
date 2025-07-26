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
std::unordered_map<socket_t, std::mutex> ChatProtocol::sendQueueMutexes;

std::queue<Chunk> &ChatProtocol::getSendQueue(int fd)
{
    return ChatProtocol::sendQueueMap.try_emplace(fd).first->second;
}

std::mutex &ChatProtocol::getMutex(int fd)
{
    return ChatProtocol::sendQueueMutexes.try_emplace(fd).first->second;
}

bool ChatProtocol::enqueueBytesUnlocked(int fd, const void *ptr, size_t size)
{
    try
    {
        std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);

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

bool ChatProtocol::enqueueBytes(int fd, const void *ptr, size_t size)
{
    std::lock_guard lock(getMutex(fd));

    return enqueueBytesUnlocked(fd, ptr, size);
}

std::unique_ptr<Chunk> ChatProtocol::dequeueChunk(int fd)
{
    auto &queue = getSendQueue(fd);

    if (queue.empty())
        return nullptr;

    auto up = std::make_unique<Chunk>(std::move(queue.front()));
    queue.pop();
    return up;
}

bool ChatProtocol::writeIntUnlocked(socket_t fd, const int data)
{
    return ChatProtocol::enqueueBytesUnlocked(fd, &data, sizeof(data));
}

bool ChatProtocol::writeInt(socket_t fd, const int data)
{
    std::lock_guard lock(getMutex(fd));
    return ChatProtocol::enqueueBytesUnlocked(fd, &data, sizeof(data));
}

bool ChatProtocol::readInt(socket_t fd, int &data)
{
    int out;
    if (recv(fd, reinterpret_cast<char *>(&out), sizeof(out), 0) < sizeof(out))
        return false;

    data = out;
    return true;
}

bool ChatProtocol::writeStringUnlocked(socket_t fd, const std::string &str)
{
    size_t ipLen = str.length();
    if (!ChatProtocol::enqueueBytesUnlocked(fd, &ipLen, sizeof(ipLen)))
    {
        std::cout << "Failed to enqueue bytes for len" << std::endl;
        return false;
    }

    if (ipLen > 0)
    {
        if (!ChatProtocol::enqueueBytesUnlocked(fd, &str[0], ipLen))
        {
            return false;
        }
    }

    return true;
}

bool ChatProtocol::writeString(socket_t fd, const std::string &str)
{
    std::lock_guard lock(getMutex(fd));

    return writeStringUnlocked(fd, str);
}

bool ChatProtocol::writeServer(socket_t fd, const Server &server)
{
    std::lock_guard lock(getMutex(fd));
    if (!writeIntUnlocked(fd, server.socket))
        return false;
    // Send IP
    if (!writeStringUnlocked(fd, server.ip))
        return false;

    // Send ID
    if (!writeStringUnlocked(fd, server.identifier))
        return false;

    // Send timestamp
    if (!enqueueBytesUnlocked(fd, &server.connectedTimestamp, sizeof(server.connectedTimestamp)))
        return false;

    return true;
}

bool ChatProtocol::readString(socket_t fd, std::string &str)
{
    ssize_t len = 0;
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

    if (!writeIntUnlocked(fd, client.socket))
        return false;
    // Send IP
    if (!writeStringUnlocked(fd, client.ip))
        return false;

    // Send ID
    if (!writeStringUnlocked(fd, client.identifier))
        return false;

    // Send username
    if (!writeStringUnlocked(fd, client.username))
        return false;

    // Send timestamp
    if (!enqueueBytesUnlocked(fd, &client.connectedTimestamp, sizeof(client.connectedTimestamp)))
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
    }

    return true;
}