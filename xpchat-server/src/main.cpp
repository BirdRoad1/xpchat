#include <iostream>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <unistd.h>
#include "server_list.h"
#include <xpchat/chat_protocol.h>
#include "central.h"
#include "server.h"

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        std::string type(argv[1]);
        std::transform(type.begin(), type.end(), type.begin(),
                       [](unsigned char c)
                       { return std::tolower(c); });

        if (type == "central")
        {
            return mainCentral();
        }
    }

    if (argc != 2) {
        std::cout << "Usage: xpchatserver <public ip>" << std::endl;
        return 1;
    }

    std::string ip = std::string(argv[1]);

    return mainServer(ip);
}