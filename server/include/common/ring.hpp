#pragma once

#include "loguru/loguru.hpp"

// reference https://code.haxe.org/category/data-structures/ring-array.html
// reference https://github.com/torvalds/linux/blob/master/include/linux/circ_buf.h

template <typename T>
class Ring
{
    int head = 0;
    int tail = 0;
    int len = 0;
    int cap = 0;
    int head_id = 0;
    T *content = nullptr;
    bool *present = nullptr;

    T *_get(int i);
    void _set(int i, T v);
    bool _present(int i);

    void push(T elt);
    void skip();

public:
    Ring(int len);
    ~Ring();

    void reset();

    int size();
    int space();

    T *getById(int id);
    T *get(int i); // i-th from last

    bool mem(int id);

    void write(int id, T elt);

    void makeAck(bool *ack);
};

template <typename T>
Ring<T>::Ring(int length)
{
    if (length < 4)
    {
        len = 4;
    }
    else if ((length & (length - 1)) > 0)
    {
        len = length - 1;
        len |= len >> 1;
        len |= len >> 2;
        len |= len >> 4;
        len |= len >> 8;
        len |= len >> 16;
        len++;
    }
    else
        len = length;

    cap = len - 1;
    content = (T *)malloc(len * sizeof(T));
    present = (bool *)malloc(len * sizeof(bool));
    for (int i = 0; i < len; i++)
        present[i] = false;
}

template <typename T>
Ring<T>::~Ring()
{
    free(content);
}

template <typename T>
T *Ring<T>::_get(int i)
{
    if (i < 0 || i > cap)
    {
        LOG_F(ERROR, "_get: index %d out of bounds %d, %d", i, 0, cap);
        return nullptr;
    }

    if (present[i])
        return &content[i];

    return nullptr;
}

template <typename T>
void Ring<T>::_set(int i, T v)
{
    if (i < 0 || i > cap)
    {
        LOG_F(ERROR, "_set: index %d out of bounds %d, %d", i, 0, cap);
        return;
    }

    content[i] = v;
    present[i] = true;
}

template <typename T>
bool Ring<T>::_present(int i)
{
    if (i < 0 || i > cap)
        return false;
    return present[i];
}

template <typename T>
void Ring<T>::push(T v)
{
    if (space() == 0)
        tail = (tail + 1) & cap;

    content[head] = v;
    present[head] = true;
    head = (head + 1) & cap;
}

template <typename T>
void Ring<T>::skip()
{
    if (space() == 0)
        tail = (tail + 1) & cap;

    present[head] = false;
    head = (head + 1) & cap;
}

template <typename T>
void Ring<T>::reset()
{
    head = 0;
    tail = 0;
}

template <typename T>
int Ring<T>::size()
{
    return (head - tail) & cap;
}

template <typename T>
int Ring<T>::space()
{
    return (tail - head - 1) & cap;
}

template <typename T>
T *Ring<T>::getById(int id)
{
    if (size() <= 0 || id > head_id || id <= head_id - cap)
        return nullptr;

    return _get((head - (head_id - id) - 1) & cap);
}

template <typename T>
T *Ring<T>::get(int i)
{
    if (i > size())
        return nullptr;

    return _get((head - i - 1) & cap);
}

template <typename T>
bool Ring<T>::mem(int id)
{
    if (size() <= 0 || id > head_id || id < head_id - cap)
        return false;

    return _present((head - (head_id - id) - 1) & cap);
}

template <typename T>
void Ring<T>::write(int id, T v)
{
    if (size() == 0)
    {
        push(v);
        head_id = id;
    }
    else
    {
        if (id > head_id)
        {
            for (int i = 0; i < id - head_id - 1; i++)
                skip();
            push(v);
            head_id = id;
        }
        else if (id >= head_id - cap)
        {
            _set((head - (head_id - id) - 1) & cap, v);

            if (head_id - id > size())
                tail = (head - (head_id - id) - 1) & cap;
        }
    }
}

template <typename T>
void Ring<T>::makeAck(bool *ack)
{
    for (int i = 0; i <= cap; i++)
        ack[i] = _present((head - i - 1) & cap);
}