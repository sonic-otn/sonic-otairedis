#include "otai_redis.h"

REDIS_GENERIC_QUAD(INTERFACE,interface);
REDIS_GENERIC_STATS(INTERFACE,interface);

const otai_interface_api_t redis_interface_api = {
    REDIS_GENERIC_QUAD_API(interface)
    REDIS_GENERIC_STATS_API(interface)
};
