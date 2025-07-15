#pragma once

struct Peer
{
    int socket;
    std::string ip;
    std::string identifier;
    time_t connectedTimestamp;
};