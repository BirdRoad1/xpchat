#pragma once
#include <string>

struct Server
{
    int socket;
    std::string ip;
    std::string identifier;
    time_t connectedTimestamp;
};