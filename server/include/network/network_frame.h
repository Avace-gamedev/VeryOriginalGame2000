#pragma once

#include <string>
#include "common/deftypes.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 128
#endif

#define HEADER_SIZE 3

union BUFFER
{
    char staticBuffer[BUFFER_SIZE];
    char *dynamicBuffer;
};

struct NetworkFrame
{
    // set to the ID of the sender by network upon reception of a new message
    // the messages created by the server to be sent to the clients do not
    // use this value
    ID sender;
    int buffer_size;
    BUFFER buffer;

    NetworkFrame(const int len = BUFFER_SIZE);
    NetworkFrame(const char *buffer);
    ~NetworkFrame();

    void append(const void *bytes, const int len);
    void appendInt8(const int8_t i);
    void appendInt16(const int16_t i);
    void appendInt32(const int32_t i);
    void appendFloat(const float f);
    void appendString(std::string s);

    const bool dynamic() const;
    OPCODE &opcode() const;
    size_t &size() const;
    size_t totalSize() const; // including header
    char *header();
    const char *header() const;
    char *content();
    const char *content() const;

    static OPCODE &NetworkFrame::getMessageOpCode(const char *message);
    static size_t &getMessageSize(const char *message);
    static size_t getMessageTotalSize(const char *message);
    static char *getMessageHeader(char *message);
    static char *getMessageContent(char *message);
};