#include "common/bitarray.h"

#include <cstring>
#include <fstream>

#include "loguru/loguru.hpp"

// https://electronics.stackexchange.com/questions/200055/most-efficient-way-of-creating-a-bool-array-in-c-avr

BitArray::BitArray(const int length) : length(length)
{
    int actual_length = ceil((double)length / 8);
    array = (uint8_t *)malloc(actual_length);
    for (int i = 0; i < actual_length; i++)
        array[i] = 0;
}

BitArray::~BitArray()
{
    free(array);
}

const bool BitArray::get(const int address) const
{
    return (array[address / 8] & (1 << (address % 8))) != 0;
}

void BitArray::set(const int address, const bool value)
{
    if (value)
        array[address / 8] |= 1 << (address % 8);
    else
        unset(address);
}

void BitArray::unset(const int address)
{
    array[address / 8] &= ~(1 << (address % 8));
}

const int BitArray::size() const { return length; }

void BitArray::fill(bool b)
{
    memset(array, b ? 255 : 0, ceil((double)length / 8));
}

const int BitArray::blitIn(char *buffer) const
{
    int actual_length = ceil((double)length / 8);
    memcpy(buffer, array, actual_length);
    return actual_length;
}

char *readFileBytes(const char *name, size_t *len)
{
    std::ifstream file(name, std::ios::binary);
    file.seekg(0, std::ios::end);
    *len = file.tellg();

    LOG_F(INFO, "reading file %s of size %d bytes", name, *len);

    char *ret = (char *)malloc(*len);
    file.seekg(0, std::ios::beg);
    file.read(ret, *len);
    file.close();
    return ret;
}