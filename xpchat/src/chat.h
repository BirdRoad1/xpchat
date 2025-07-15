#pragma once
#include <xpchat/server.h>
#include <queue>

class Chat
{
private:
    Chat() {}
    ~Chat() {}
    int fd = -1;
    Server server {};
    

public:
    static Chat &getInstance()
    {
        static Chat instance;

        return instance;
    }

    Chat &operator=(const Chat &) = delete;
    Chat(const Chat &) = delete;
    bool disconnect();

    bool connect(const Server server);
    const Server* getActiveServer();
};