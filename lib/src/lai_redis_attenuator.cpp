#include "lai_redis.h"

REDIS_GENERIC_QUAD(ATTENUATOR,attenuator);
REDIS_GENERIC_STATS(ATTENUATOR,attenuator);

const lai_attenuator_api_t redis_attenuator_api = {
    REDIS_GENERIC_QUAD_API(attenuator)
    REDIS_GENERIC_STATS_API(attenuator)
};
