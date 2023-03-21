#include "lai_redis.h"

REDIS_GENERIC_QUAD(TRANSCEIVER,transceiver);
REDIS_GENERIC_STATS(TRANSCEIVER,transceiver);

const lai_transceiver_api_t redis_transceiver_api = {
    REDIS_GENERIC_QUAD_API(transceiver)
    REDIS_GENERIC_STATS_API(transceiver)
};

