#include "lai_redis.h"

REDIS_GENERIC_QUAD(OSC,osc);
REDIS_GENERIC_STATS(OSC,osc);

const lai_osc_api_t redis_osc_api = {
    REDIS_GENERIC_QUAD_API(osc)
    REDIS_GENERIC_STATS_API(osc)
};
