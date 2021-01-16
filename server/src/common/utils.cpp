#include "common/utils.h"

#include "loguru/loguru.hpp"
#include <fstream>

ID __id_cnt = 0;
ID freshID() { return __id_cnt++; }

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