#include "otai_redis.h"

REDIS_GENERIC_QUAD(PHYSICALCHANNEL,physicalchannel);
REDIS_GENERIC_STATS(PHYSICALCHANNEL,physicalchannel);

const otai_physicalchannel_api_t redis_physicalchannel_api = {
    REDIS_GENERIC_QUAD_API(physicalchannel)
    REDIS_GENERIC_STATS_API(physicalchannel)
};
