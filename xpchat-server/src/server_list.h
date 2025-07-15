#pragma once

#include <xpchat/server.h>
#include "peer_list.h"

class ServerList : public PeerList<Server>
{
public:
    static ServerList &getInstance()
    {
        static ServerList instance;

        return instance;
    }
};