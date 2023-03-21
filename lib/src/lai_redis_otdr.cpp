#include "lai_redis.h"

REDIS_GENERIC_QUAD(OTDR,otdr);
REDIS_GENERIC_STATS(OTDR,otdr);

const lai_otdr_api_t redis_otdr_api = {
    REDIS_GENERIC_QUAD_API(otdr)
    REDIS_GENERIC_STATS_API(otdr)
};
