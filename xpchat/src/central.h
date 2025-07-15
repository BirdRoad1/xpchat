#include <vector>
#include <xpchat/server.h>

class Central
{
private:
    Central() {}
    ~Central() {}
    static int fd;

public:
    static Central &getInstance()
    {
        static Central instance;

        return instance;
    }
    Central &operator=(const Central &) = delete;
    Central(const Central &) = delete;

    bool connect();
    bool listServers(std::vector<Server>& servers);
    bool sendServerJoin(int id);

};