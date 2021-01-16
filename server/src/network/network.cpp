#include "network/network.h"

#include <windows.h>
#include <time.h>

#include "loguru/loguru.hpp"

#include "common/time.h"
#include "common/utils.h"

UDPServer::UDPServer(int port)
{
    int errcode;

    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    // create socket
    this->sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (this->sock == INVALID_SOCKET)
    {
        LOG_F(ERROR, "socket: %d", WSAGetLastError());
        close();
        return;
    }
    sock_open = true;

    // bind to some address
    errcode = bind(this->sock, (sockaddr *)&addr, sizeof(addr));
    if (errcode == SOCKET_ERROR)
    {
        LOG_F(ERROR, "bind: %d", WSAGetLastError());
        close();
        return;
    }

    server_open = true;

    LOG_F(INFO, "Listening on port %d", port);
}

int UDPServer::_send(const char *buffer, int buffer_size, const sockaddr_in *to) const
{
    int bytes_sent = sendto(sock, buffer, buffer_size, 0, (sockaddr *)to, sizeof(sockaddr_in));
    if (bytes_sent == SOCKET_ERROR)
    {
        LOG_F(ERROR, "sent to %s:%hu: %d", inet_ntoa(to->sin_addr), ntohs(to->sin_port), WSAGetLastError());
        return 0;
    }

    return bytes_sent;
}

int UDPServer::sendTo(ID id, const NetworkFrame &frame)
{
    if (!isAlive(id))
    {
        LOG_F(ERROR, "cannot send to dead peer %d", id);
        return -1;
    }

    sockaddr_in addr;
    if (!getAddr(id, &addr))
        return 0;

    return _send(frame.header(), frame.totalSize(), &addr);
}

int UDPServer::_receive(char *buffer, int buffer_size, sockaddr_in *from)
{
    int from_len = sizeof(sockaddr_in);
    int bytes_received = recvfrom(sock, buffer, buffer_size, 0, (sockaddr *)from, &from_len);

    int i = getPeerSlotByAddr(from);
    if (bytes_received == SOCKET_ERROR)
    {
        if (WSAGetLastError() == WSAECONNRESET)
        {
            if (i >= 0)
            {
                LOG_F(ERROR, "received from %s:%hu: WSAECONNRESET, dropping peer", inet_ntoa(from->sin_addr), ntohs(from->sin_port));
                ID id = peers[i].id;
                kill(peers[i].id);
            }
        }
        else
            LOG_F(ERROR, "received from %s:%hu: %d", inet_ntoa(from->sin_addr), ntohs(from->sin_port), WSAGetLastError());

        return -WSAGetLastError();
    }
    else if (i >= 0)
        last_com_date[i] = Time::nowInMilliseconds();

    return bytes_received;
}

int UDPServer::update(unsigned long long timeout)
{
    // drop dead clients
    for (int i = 0; i < MAX_PEERS; i++)
    {
        if (alive[i] &&
            Time::nowInMilliseconds() > last_com_date[i] + SERVER_TIMEOUT)
        {
            LOG_F(ERROR, "no frame from peer %d at %s:%hu for %d ms, dropping peer",
                  peers[i].id,
                  inet_ntoa(peers[i].addr.sin_addr), ntohs(peers[i].addr.sin_port),
                  Time::nowInMilliseconds() - last_com_date[i]);
            kill(peers[i].id);
        }
    }

    struct timeval tv = Time::timevalOfLongLong(timeout);

    FD_SET read_set;
    FD_ZERO(&read_set);
    FD_SET(sock, &read_set);

    int ret = select(0, &read_set, nullptr, nullptr, &tv);

    if (ret > 0)
    {
        char buffer[BUFFER_SIZE];
        sockaddr_in from;
        int received = _receive(buffer, BUFFER_SIZE, &from);

        if (received <= 0)
            return 0;

        int i = 0;
        while (i < received)
        {
            int expected_length = NetworkFrame::getMessageSize(&buffer[i]);

            if (expected_length < 0)
            {
                LOG_F(ERROR, "expected length >= 0, discarding remaining bytes");
                return i;
            }

            if (i + expected_length + HEADER_SIZE > received)
            {
                LOG_F(ERROR, "received partial message, discarding remaining bytes");
                return i;
            }

            NetworkFrame frame(&buffer[i]);
            int player_slot = getPeerSlotByAddr(&from);

            switch (frame.opcode())
            {
            case OP_INIT:
                doClientInit(frame, &from);
                break;
            case OP_PING:
            {
                if (player_slot >= 0)
                {
                    NetworkFrame pong;
                    pong.opcode() = OP_PONG;
                    pong.append(&peers[player_slot].id, sizeof(ID));
                    sendTo(peers[player_slot].id, pong);
                }
                else
                    LOG_F(WARNING, "received ping message from unknown address %s:%hu", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
                break;
            }
            case OP_PONG:
                break;
            default:
                if (player_slot >= 0)
                {
                    frame.sender = peers[player_slot].id;
                    msg_queue.push(frame);
                }
                else
                    LOG_F(WARNING, "received non-init message from unknown address %s:%hu", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
            }

            i += expected_length + HEADER_SIZE;
        }

        return i;
    }

    return 0;
}

int UDPServer::getAvailableSlot() const
{
    for (int i = 0; i < MAX_PEERS; i++)
        if (!alive[i])
            return i;
    return -1;
}

Peer *UDPServer::setSlot(int i, ID id, const sockaddr_in *addr)
{
    if (alive[i])
    {
        LOG_F(ERROR, "attempt to overwrite living peer %d by peer %d (ignored)", peers[i].id, id);
        return nullptr;
    }

    peers[i].id = id;
    peers[i].addr = *addr;
    alive[i] = true;
    last_com_date[i] = Time::nowInMilliseconds();

    return &peers[i];
}

int UDPServer::getPeerSlotByID(const ID id) const
{
    for (int i = 0; i < MAX_PEERS; i++)
    {
        if (alive[i] && peers[i].id == id)
            return i;
    }
    return -1;
}

int UDPServer::getPeerSlotByAddr(const sockaddr_in *addr) const
{
    for (int i = 0; i < MAX_PEERS; i++)
    {
        if (alive[i] && (addr->sin_addr.s_addr == peers[i].addr.sin_addr.s_addr) && (addr->sin_port == peers[i].addr.sin_port))
            return i;
    }
    return -1;
}

bool UDPServer::isAlive(const ID id) const
{
    int i = getPeerSlotByID(id);
    if (i < 0)
        return false;
    return true;
}

void UDPServer::kill(const ID id)
{
    int i = getPeerSlotByID(id);
    if (i >= 0)
    {
        alive[i] = false;
        lost_connections.push_back(id);
    }
    else
        LOG_F(WARNING, "cannot kill dead peer %d (ignored)", id);
}

bool UDPServer::getAddr(const ID id, sockaddr_in *addr) const
{
    int i = getPeerSlotByID(id);
    if (i >= 0)
    {
        *addr = peers[i].addr;
        return true;
    }

    LOG_F(ERROR, "asking for address of unknown peer %d", id);
    return false;
}

bool UDPServer::doClientInit(NetworkFrame frame, sockaddr_in *from)
{
    static char init_msg[8] = "hithere";

    if (!OPCODE_IS(frame.opcode(), OP_INIT))
        return false;

    int expected_length = frame.size();
    const char *content = frame.content();

    // add \0 in case its not there
    char str[BUFFER_SIZE];
    memcpy(str, content, 7);
    str[7] = '\0';

    if (strcmp(str, init_msg) != 0)
        return false;

    Peer *peer;

    int i = getPeerSlotByAddr(from);

    if (i >= 0)
    {
        peer = &peers[i];
        LOG_F(INFO, "reinit from old peer %d", peer->id);
    }
    else
    {
        int i = getAvailableSlot();
        peer = setSlot(i, freshID(), from);

        LOG_F(INFO, "assigned ID %d to new peer %s:%hu", peer->id, inet_ntoa(peer->addr.sin_addr), ntohs(peer->addr.sin_port));
    }

    sendServerInit(peer->id);

    return true;
}

void UDPServer::sendServerInit(const ID id)
{
    static char init_msg[8] = "hithere";
    static int timeout_ms = SERVER_TIMEOUT;

    NetworkFrame frame;
    frame.append(init_msg, 7);
    frame.append(&id, sizeof(&id));
    frame.append(&timeout_ms, sizeof(timeout_ms));
    frame.opcode() = OP_INIT;

    sendTo(id, frame);
}

std::vector<ID> UDPServer::lostConnections()
{
    if (lost_connections.size() <= 0)
        return std::vector<ID>();

    std::vector<ID> connections(lost_connections);
    lost_connections.clear();
    return connections;
}

bool UDPServer::empty() const
{
    return msg_queue.empty();
}

// copy last message, it will be removed from internal buffer after call to `discardReadMessages`
NetworkFrame UDPServer::pop()
{
    NetworkFrame frame = msg_queue.front();
    msg_queue.pop();
    return frame;
}

void UDPServer::close()
{
    if (sock_open)
        closesocket(sock);

    sock_open = false;
    server_open = false;

    LOG_F(INFO, "Server closed.");
}

//

TCPServer::TCPServer(int port, std::vector<NetworkFrame *> files) : files(files), current_file(0), sent(0)
{
    int errcode;

    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    // create socket
    this->sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (this->sock == INVALID_SOCKET)
    {
        LOG_F(ERROR, "TCP socket: %d", WSAGetLastError());
        close();
        return;
    }

    sock_open = true;

    // bind to some address
    errcode = bind(this->sock, (sockaddr *)&addr, sizeof(addr));
    if (errcode == SOCKET_ERROR)
    {
        LOG_F(ERROR, "TCP bind: %d", WSAGetLastError());
        close();
        return;
    }

    errcode = listen(this->sock, 1);
    if (errcode == SOCKET_ERROR)
    {
        LOG_F(ERROR, "TCP listen: %d", WSAGetLastError());
        close();
        return;
    }

    server_open = true;
    start_time = Time::now();

    LOG_F(INFO, "TCP SOCKET listening on port %d", port);
}

bool TCPServer::isOpen() const { return server_open; }
int TCPServer::port() const { return ntohs(addr.sin_port); }
int TCPServer::totalSize() const
{
    int size = 0;
    for (NetworkFrame *file : files)
        size += file->size();
    return size;
}

bool TCPServer::update(unsigned long long timeout)
{
    if (!isOpen())
    {
        LOG_F(ERROR, "TCP update: server closed");
        return true;
    }

    struct timeval tv = Time::timevalOfLongLong(timeout);

    if (!client_connected)
    {
        int sock_len = sizeof(sockaddr_in);

        FD_SET read_set;
        FD_ZERO(&read_set);
        FD_SET(sock, &read_set);

        int ret = select(0, &read_set, nullptr, nullptr, &tv);

        if (ret > 0)
        {
            client_sock = accept(sock, (sockaddr *)&client, &sock_len);

            if (client_sock == INVALID_SOCKET)
            {
                LOG_F(ERROR, "TCP accept: %d, %d", WSAGetLastError(), client_sock);
                return false;
            }
            else
            {
                LOG_F(INFO, "Established connection with %s:%hu, sending %d bytes", inet_ntoa(client.sin_addr), ntohs(client.sin_port), totalSize());
                client_connected = true;
            }
        }
        else
        {
            unsigned long long t = Time::now() - start_time;
            if (t > SERVER_TIMEOUT)
            {
                // close the server if no client has shown up
                LOG_F(ERROR, "client has not showed up for %llu, closing", t);
                return true;
            }
        }
    }
    else
    {

        FD_SET write_set;
        FD_ZERO(&write_set);
        FD_SET(client_sock, &write_set);

        int ret = select(0, nullptr, &write_set, nullptr, &tv);

        if (ret > 0)
        {
            int remaining = files[current_file]->totalSize() - sent;
            int packet_size = min(TCP_PACKET_SIZE, remaining);
            int written = send(client_sock, &files[current_file]->header()[sent], packet_size, 0);
            if (written == SOCKET_ERROR)
            {
                int error = WSAGetLastError();
                if (error == WSAECONNRESET)
                {
                    LOG_F(ERROR, "TCP send: WSAECONNRESET, closing server");
                    return true;
                }
                else
                {
                    LOG_F(ERROR, "TCP send: %d", WSAGetLastError());
                    return true;
                }
            }
            else
                sent += written;

            if (sent >= files[current_file]->totalSize())
            {
                current_file++;
                sent = 0;
            }

            return current_file >= files.size();
        }
    }

    return false;
}

void TCPServer::close()
{
    if (sock_open)
        closesocket(sock);

    sock_open = false;
    server_open = false;

    LOG_F(INFO, "TCP server closed.");
}