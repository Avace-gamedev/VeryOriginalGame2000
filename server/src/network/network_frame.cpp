#include "network/network_frame.h"

#include "loguru/loguru.hpp"

NetworkFrame::NetworkFrame(const framesize_t len)
{
#if DEBUG
    if (len > 1 << (4 * sizeof(framesize_t)) - 1)
        LOG_F(ERROR, "len %d too big, field size is only int%d", len, sizeof(framesize_t) * 4);
#endif

    buffer_size = BUFFER_SIZE;

    if (len > BUFFER_SIZE)
    {
        buffer.dynamicBuffer = (char *)malloc(len);

        if (buffer.dynamicBuffer == nullptr)
            LOG_F(ERROR, "could not preallocate %d bytes", len);
        else
            buffer_size = len;
    }

    sender = -1;
    opcode() = 0;
    size() = 0;
}

NetworkFrame::NetworkFrame(const char *buffer) : NetworkFrame(NetworkFrame::getMessageSize(buffer) + HEADER_SIZE)
{
    memcpy(header(), buffer, NetworkFrame::getMessageSize(buffer) + HEADER_SIZE);
}

NetworkFrame::~NetworkFrame()
{
    if (dynamic())
        free(buffer.dynamicBuffer);
}

const bool NetworkFrame::dynamic() const
{
    return buffer_size > BUFFER_SIZE;
}

OPCODE &NetworkFrame::opcode() const
{
    return NetworkFrame::getMessageOpCode(header());
}

framesize_t NetworkFrame::totalSize() const
{
    return NetworkFrame::getMessageTotalSize(header());
}

framesize_t &NetworkFrame::size() const
{
    return NetworkFrame::getMessageSize(header());
}

char *NetworkFrame::header()
{
    if (dynamic())
        return buffer.dynamicBuffer;
    else
        return buffer.staticBuffer;
}

const char *NetworkFrame::header() const
{
    if (dynamic())
        return buffer.dynamicBuffer;
    else
        return buffer.staticBuffer;
}

char *NetworkFrame::content()
{
    return NetworkFrame::getMessageContent(header());
}

const char *NetworkFrame::content() const
{
    return &header()[HEADER_SIZE];
}

void NetworkFrame::append(const void *bytes, const framesize_t len)
{
    if (HEADER_SIZE + size() + len > buffer_size)
    {
        // allocate twice the needed space
        int new_size = 2 * (HEADER_SIZE + size() + len);
        if (dynamic())
        {
            // reallocate dynamic array
            buffer.dynamicBuffer = (char *)realloc(buffer.dynamicBuffer, new_size);
        }
        else
        {
            // allocate new dynamic array, and copy previous content
            char *new_buffer = (char *)malloc(new_size);
            memcpy(new_buffer, buffer.staticBuffer, BUFFER_SIZE);
            buffer.dynamicBuffer = new_buffer;
        }
        buffer_size = new_size;
    }

    // now buffer is big enough
    char *c = content();
    memcpy(&c[size()], bytes, len);
    size() += len;
}

void NetworkFrame::appendInt8(const int8_t i)
{
    append((char *)&i, 1);
}

void NetworkFrame::appendInt16(const int16_t i)
{
    append((char *)&i, 2);
}

void NetworkFrame::appendInt32(const int32_t i)
{
    append((char *)&i, 4);
}

void NetworkFrame::appendFloat(const float f)
{
    append((char *)&f, 4);
}

void NetworkFrame::appendString(std::string s)
{
    const char *cstr = s.c_str();
    int len = (int)strlen(cstr);
    append(cstr, len + 1);
}

OPCODE &NetworkFrame::getMessageOpCode(const char *message)
{
    return ((OPCODE *)message)[0];
}

framesize_t &NetworkFrame::getMessageSize(const char *message)
{
    return ((framesize_t *)(&message[1]))[0];
}

framesize_t NetworkFrame::getMessageTotalSize(const char *message)
{
    return getMessageSize(message) + HEADER_SIZE;
}

char *NetworkFrame::getMessageContent(char *message)
{
    return &message[HEADER_SIZE];
}