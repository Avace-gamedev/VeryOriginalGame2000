#include "network/network.h"

#include "loguru/loguru.hpp"
#include "network/portfinder.h"
#include "network/network.h"
#include "common/time.h"

#define PERIOD 1000.0 / 60.0 // 60 frame per sec, in msec

int main(int argc, char **argv)
{
    loguru::init(argc, argv);
    Time::startNow();

    int errcode;

    WSADATA wsaData;
    errcode = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (errcode != NO_ERROR)
    {
        LOG_F(ERROR, "WSAStartup: %d", errcode);
        return 1;
    }

    // BuggyUDPServer network(8890, 500, 200, 0.1);
    UDPServer network(8890);
    if (!network.isOpen())
    {
        WSACleanup();
        return 1;
    }

    PortFinder<256> port_finder(62000);

    // prepare files to be sent by TCP servers

    std::vector<NetworkFrame *> files;
    NetworkFrame big_file(10000);
    files.push_backq(&big_file);

    while (network.isOpen())
    {
        unsigned long long timeout = Time::timeBeforeDeadline(PERIOD) / 2;
        if (timeout > 0)
            network.update(timeout);

        // update players
        while (!network.empty())
        {
            NetworkFrame message = network.pop();

            // do something with message
        }
    }

    network.close();
    WSACleanup();

    return 0;
}