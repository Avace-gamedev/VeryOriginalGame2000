#include "common/utils.h"

ID __id_cnt = 0;
ID freshID() { return __id_cnt++; }