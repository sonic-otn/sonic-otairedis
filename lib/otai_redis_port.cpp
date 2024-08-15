#include "otai_redis.h"

REDIS_GENERIC_QUAD(PORT,port);
REDIS_GENERIC_STATS(PORT,port);

const otai_port_api_t redis_port_api = {
    REDIS_GENERIC_QUAD_API(port)
    REDIS_GENERIC_STATS_API(port)
};
