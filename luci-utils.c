#include "luci.h"
#include <arpa/inet.h>

float ntohf(uint32_t net32)
{
    union {
        float f;
        uint32_t u;
    } value;

    value.u = ntohl(net32);

    return value.f;
}
