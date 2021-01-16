#include <filesystem>
#include <memory>
#include "loguru/loguru.hpp"

#include "common/deftypes.h"
#include "common/time.h"
#include "common/vector.hpp"
#include "common/utils.h"
#include "network/portfinder.h"
#include "network/network.h"
#include "engine/game_config.h"
#include "engine/player.h"
#include "engine/weapon.h"
#include "engine/tilemap.h"
#include "engine/world.h"
#include "engine/controller.h"
#include "engine/ai.h"

#define PERIOD 1000.0 / 60.0 // 60 frame per sec, in msec

namespace fs = std::filesystem;

void handleFrame(const NetworkFrame &frame);

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

    // make map
    Map map(4);
    map.load("data/second_try.xml");

    NetworkFrame tilemap_frame(map.getTilemap()->size());
    tilemap_frame.opcode() = OP_BINARY;
    map.getTilemap()->write(tilemap_frame);

    NetworkFrame tileset_frame(map.getTileset()->size());
    tileset_frame.opcode() = OP_BINARY;
    map.getTileset()->write(tileset_frame);

    // load weapons

    Weapons::loadFromFile("data/weapons.xml");
    NetworkFrame weapons_frame(Weapons::size());
    LOG_F(INFO, "Loaded %d weapons", Weapons::nWeapons());
    weapons_frame.opcode() = OP_BINARY;
    Weapons::write(weapons_frame);

    // prepare files to be sent by TCP servers

    std::vector<NetworkFrame *> files;
    files.push_back(&tilemap_frame);
    files.push_back(&tileset_frame);
    files.push_back(&weapons_frame);

    // make world
    World world(&map);

    // create AI players
    std::shared_ptr<LinePath> ai = std::make_shared<LinePath>(&world, 200.0);

    AIController *controller = new AIController(ai);
    Entity other_player(freshID(), CONTROLLED_ENTITY, "Phasko", 100, controller);
    other_player.type = PLAYER;
    other_player.move(Vec2f(100, 100));
    world.add(&other_player);

    AIController *controller_ = new AIController(ai);
    Entity other_player_(freshID(), CONTROLLED_ENTITY, "Mr. le Bref", 100, controller_);
    other_player_.type = PLAYER;
    other_player_.move(Vec2f(200, 400));
    world.add(&other_player_);

    // initial snapshot
    Snapshot snapshot = world.makeSnapshot(0);
    world.remember(snapshot);

    tick_t last_server_tick = Time::nowInTicks(SERVER_PERIOD);
    tick_t last_client_tick = Time::nowInTicks(CLIENT_PERIOD);

    std::vector<ID> new_connections;
    std::vector<TCPServer *> file_loaders;

    unsigned long long infrequent_log_deadline = 0;

    while (network.isOpen())
    {
        tick_t server_tick = Time::nowInTicks(SERVER_PERIOD);
        tick_t client_tick = Time::nowInTicks(CLIENT_PERIOD);

        if (Time::nowInMilliseconds() > infrequent_log_deadline)
        {
            LOG_F(INFO, "Tick %d, n_players:%d, n_entities:%d", Time::nowInTicks(CLIENT_PERIOD), world.getNPlayers(), world.getNEntities());
            infrequent_log_deadline = Time::nextDeadline(600 * SERVER_PERIOD);
        }

        // read messages, update lost connections
        // return in less than timeout ms
        unsigned long long timeout = Time::timeBeforeDeadline(CLIENT_PERIOD) / 2;
        if (timeout > 0)
            network.update(timeout);

        // drop dead players
        for (auto player_id : network.lostConnections())
            world.dropPlayer(player_id);

        // update players
        while (!network.empty())
        {
            NetworkFrame frame = network.pop();

            // handle incoming frame
            switch (frame.opcode())
            {
            case OP_CONTROL_FRAME:
            {
                Player *player = world.getPlayerById(frame.sender);
                if (!player)
                {
                    LOG_F(ERROR, "cannot find player with id %d", frame.sender);
                    continue;
                }

                ControlFrame ctrl_frame = ControlFrame::read(frame);
                player->rememberControl(ctrl_frame.control);
                break;
            }
            case OP_STATIC_INFO:
            {
                LOG_F(INFO, "launching TCP server");
                TCPServer *tcp_server;
                int port;
                do
                {
                    port = port_finder.get();
                    LOG_F(INFO, "Trying port %d...", port);
                    tcp_server = new TCPServer(port, files);
                } while (!tcp_server->isOpen());

                file_loaders.push_back(tcp_server);

                // write some info to be sent through UDP along with TCP port

                NetworkFrame new_frame;
                new_frame.appendInt32((int)files.size());
                new_frame.appendInt32(port);
                new_frame.opcode() = OP_STATIC_INFO;
                network.sendTo(frame.sender, new_frame);
                break;
            }
            case OP_CONFIG:
            {
                sockaddr_in player_addr;
                if (!network.getAddr(frame.sender, &player_addr))
                {
                    LOG_F(ERROR, "unknown player %d at %s:%hu asking for world (ignored)",
                          frame.sender, inet_ntoa(player_addr.sin_addr), ntohs(player_addr.sin_port));
                    continue;
                }

                // make sure the name is not too long and
                // that the string is null terminated
                char *player_name = frame.content();
                if (frame.size() <= NAME_SIZE)
                    player_name[frame.size()] = '\0';
                else
                {
                    NetworkFrame frame;
                    frame.opcode() = OP_WRONG_CONFIG;
                    network.sendTo(frame.sender, frame);
                    continue;
                }

                Player *player = world.createPlayer(frame.sender, player_addr, player_name);
                LOG_F(INFO, "created new player %s with ID %d", player->name, player->id);
                new_connections.push_back(player->id);
                break;
            }
            case OP_CLIENT_READY:
            {
                Player *player = world.getPlayerById(frame.sender);
                if (!player)
                {
                    LOG_F(ERROR, "unknown player %d is ready (ignored)",
                          frame.sender);
                    continue;
                }
                player->ready = true;
                break;
            }
            default:
                LOG_F(WARNING, "[tick:%d] Got unexpected frame while waiting for CONTROL_FRAME: opcode:%hu size:%hu (dropped)", Time::nowInTicks(CLIENT_PERIOD), frame.opcode(), frame.size());
            }
        }

        // do frequent stuff:
        // read and process incoming control frames from players
        // send new packets from tcp servers
        if (client_tick > last_client_tick)
        {
            world.update(client_tick);

            for (int i = 0; i < file_loaders.size(); i++)
                if (file_loaders[i]->update(1))
                {
                    file_loaders[i]->close();
                    port_finder.setFree(file_loaders[i]->port());
                    delete file_loaders[i];
                    file_loaders.erase(file_loaders.begin() + i);
                    i--;
                }

            last_client_tick = client_tick;
        }

        // do less frequent stuff
        // send snapshot to players and world configs to new players
        if (server_tick > last_server_tick)
        {
            if (server_tick > last_server_tick + 1)
                LOG_F(WARNING, "Missed server ticks (%d -> %d)", last_server_tick, server_tick);

            // produce snapshot for current tick
            Snapshot snapshot = world.makeSnapshot(client_tick);
            world.remember(snapshot);

            if (new_connections.size() > 0)
            {
                WorldConfig config;
                config.client_rate = CLIENT_RATE;
                config.server_rate = SERVER_RATE;
                config.ack_size = ACK_SIZE;
                config.initial_snapshot = &snapshot;

                for (ID id : new_connections)
                {
                    Player *player = world.getPlayerById(id);
                    if (!player)
                    {
                        LOG_F(ERROR, "player %d disconnected before I could send world config :(", id);
                        continue;
                    }

                    NetworkFrame frame(config.size());
                    config.write(frame, player, map.getTilemap());
                    network.sendTo(id, frame);

                    LOG_F(INFO, "sent %d bytes to player %d, initial tick %d", frame.size(), id, config.initial_snapshot->tick);
                }
                new_connections.clear();
            }

            for (Player *player : world.getPlayers())
            {
                if (player->ready && network.isAlive(player->id))
                {
                    NetworkFrame frame(snapshot.size());
                    snapshot.write(frame, player, map.getTilemap());
                    network.sendTo(player->id, frame);
                }
            }

            last_server_tick = server_tick;
        }
    }

    network.close();
    WSACleanup();

    return 0;
}