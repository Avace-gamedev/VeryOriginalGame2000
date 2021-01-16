#pragma once

#include <windows.h>
#include <vector>
#include <queue>

#include "common/deftypes.h"
#include "network_frame.h"

#ifndef MAX_PEERS
#define MAX_PEERS 50
#endif

#ifndef SERVER_TIMEOUT
#define SERVER_TIMEOUT 5000 // 5 secs
#endif

#ifndef TCP_PACKET_SIZE
#define TCP_PACKET_SIZE 1024
#endif

#define OP_PING 1
#define OP_PONG 2
#define OP_BINARY 5

#define OP_INIT 11

#define OPCODE_IS(OPCODE1, OPCODE2) \
    (OPCODE1 == OPCODE2)

struct Peer
{
    ID id;
    sockaddr_in addr;
};

class UDPServer
{
protected:
    SOCKET sock;
    sockaddr_in addr;

    Peer peers[MAX_PEERS];
    unsigned long long last_com_date[MAX_PEERS]; // in ms
    bool alive[MAX_PEERS] = {false};

    std::vector<ID> lost_connections;

    ID unique_id_cnt = 0;

    bool sock_open = false;
    bool server_open = false;

    // send `buffer_size` bytes read from `buffer` to `dst`
    // first 4 bytes of sent message contain `buffer_size`
    int _send(const char *buffer, int buffer_size, const sockaddr_in *dst) const;

    // read as much data as possible from `sock`
    // you should `select` on sock before calling this
    int _receive(char *buffer, int buffer_size, sockaddr_in *from);

    int getPeerSlotByID(const ID id) const;
    int getPeerSlotByAddr(const sockaddr_in *addr) const;

    bool doClientInit(NetworkFrame frame, sockaddr_in *from);
    void sendServerInit(const ID id);

public:
    UDPServer(int port);
    std::queue<NetworkFrame> msg_queue;

    bool isOpen() const { return server_open; };

    // timeout in ms
    virtual int update(unsigned long long timeout);

    virtual int sendTo(ID id, const NetworkFrame &frame);

    std::vector<ID> lostConnections();

    bool getAddr(const ID id, sockaddr_in *addr) const;
    bool isAlive(const ID id) const;
    void kill(const ID id);

    int getAvailableSlot() const;
    virtual Peer *setSlot(int i, ID id, const sockaddr_in *addr);

    virtual bool empty() const;
    virtual NetworkFrame pop();

    void close();
};

class TCPServer
{
    SOCKET sock = INVALID_SOCKET;
    sockaddr_in addr;

    bool client_connected = false;
    sockaddr_in client;
    SOCKET client_sock = INVALID_SOCKET;

    bool sock_open = false;
    bool server_open = false;

    std::vector<NetworkFrame *> files;
    int current_file;
    int sent = 0;

    unsigned long long start_time = 0;

public:
    TCPServer(int port, std::vector<NetworkFrame *> files);

    bool isOpen() const;
    int port() const;
    int totalSize() const;

    bool update(unsigned long long timeout);
    void close();
};