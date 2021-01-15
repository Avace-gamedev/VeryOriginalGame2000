#include <stdint.h>

class BitArray
{
    uint8_t *array;
    const int length;

public:
    BitArray(const int length);
    ~BitArray();

    const bool get(const int address) const;
    void set(const int address, const bool value = true);
    void unset(const int address);

    const int size() const;

    void fill(bool b);
    const int blitIn(char *buffer) const;
};