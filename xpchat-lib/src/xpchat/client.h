#pragma once
#include <string>
#include "peer.h"

struct Client : public Peer
{
    std::string username;
};