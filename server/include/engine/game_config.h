#pragma once

#define CLIENT_RATE 60
#define CLIENT_PERIOD (1.0f / CLIENT_RATE)
#define SERVER_RATE 20
#define SERVER_PERIOD (1.0f / SERVER_RATE)
#define MAX_PING ((unsigned long long)1500) // in msec

#define NAME_SIZE 50 // max size of names
#define MAX_N_WEAPONS 4 // max number of weapon that an entity can carry
#define WALK_SPEED_MOD 0.7f // between 0 and 1
#define CHWEAP_DELAY ((unsigned long long)(1000.0f * CLIENT_PERIOD / 2)) // in msec

#define SEPARATE_BIAS 4 // cf. HaxeFlixel collision engine implementation
#define ACK_SIZE 2 // number of bytes, ie 16 frames


#define OP_STATIC_INFO 12
#define OP_CONFIG 13
#define OP_CLIENT_READY 14

#define OP_WRONG_CONFIG 21

#define OP_SNAPSHOT 100
#define OP_CONTROL_FRAME 101