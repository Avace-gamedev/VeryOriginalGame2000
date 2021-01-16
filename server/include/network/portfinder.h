// do we really need this ?? 

#include "common/bitarray.h"

template <int N>
class PortFinder
{
    int offset;
    BitArray present;

public:
    PortFinder(const int start);

    int get();

    void setBusy(const int port);
    void setFree(const int port);
};

template <int N>
PortFinder<N>::PortFinder(const int start) : offset(start), present(N)
{
    present.fill(true);
}

template <int N>
int PortFinder<N>::get()
{
    for (int i = 0; i < N; i++)
        if (present.get(i))
        {
            setBusy(i + offset);
            return i + offset;
        }
    return -1;
}

template <int N>
void PortFinder<N>::setBusy(const int port)
{
    present.set(port - offset, false);
}

template <int N>
void PortFinder<N>::setFree(const int port)
{
    present.set(port - offset, true);
}

char *readFileBytes(const char *name, int *len);