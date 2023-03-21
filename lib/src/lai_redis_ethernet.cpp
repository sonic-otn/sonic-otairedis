#include "lai_redis.h"

REDIS_GENERIC_QUAD(ETHERNET,ethernet);
REDIS_GENERIC_STATS(ETHERNET,ethernet);

const lai_ethernet_api_t redis_ethernet_api = {
    REDIS_GENERIC_QUAD_API(ethernet)
    REDIS_GENERIC_STATS_API(ethernet)
};
