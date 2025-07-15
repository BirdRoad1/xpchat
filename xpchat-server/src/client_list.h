#pragma once

#include <xpchat/client.h>
#include "peer_list.h"

class ClientList : public PeerList<Client>
{
public:
    static ClientList &getInstance()
    {
        static ClientList instance;

        return instance;
    }
};