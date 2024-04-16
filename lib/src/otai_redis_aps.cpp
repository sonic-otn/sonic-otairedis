#include "otai_redis.h"

REDIS_GENERIC_QUAD(APS,aps);
REDIS_GENERIC_STATS(APS,aps);

const otai_aps_api_t redis_aps_api = {
    REDIS_GENERIC_QUAD_API(aps)
    REDIS_GENERIC_STATS_API(aps)
};
