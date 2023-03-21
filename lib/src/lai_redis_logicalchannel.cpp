#include "lai_redis.h"

REDIS_GENERIC_QUAD(LOGICALCHANNEL,logicalchannel);
REDIS_GENERIC_STATS(LOGICALCHANNEL,logicalchannel);

const lai_logicalchannel_api_t redis_logicalchannel_api = {
    REDIS_GENERIC_QUAD_API(logicalchannel)
    REDIS_GENERIC_STATS_API(logicalchannel)
};
