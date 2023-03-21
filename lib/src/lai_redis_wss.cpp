#include "lai_redis.h"

REDIS_GENERIC_QUAD(WSS,wss);
REDIS_GENERIC_STATS(WSS,wss);

const lai_wss_api_t redis_wss_api = {
    REDIS_GENERIC_QUAD_API(wss)
    REDIS_GENERIC_STATS_API(wss)
};
