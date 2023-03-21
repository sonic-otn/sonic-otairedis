#include "lai_redis.h"

REDIS_GENERIC_QUAD(LLDP,lldp);
REDIS_GENERIC_STATS(LLDP,lldp);

const lai_lldp_api_t redis_lldp_api = {
    REDIS_GENERIC_QUAD_API(lldp)
    REDIS_GENERIC_STATS_API(lldp)
};
