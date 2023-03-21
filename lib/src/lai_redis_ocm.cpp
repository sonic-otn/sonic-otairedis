#include "lai_redis.h"

REDIS_GENERIC_QUAD(OCM,ocm);
REDIS_GENERIC_STATS(OCM,ocm);

const lai_ocm_api_t redis_ocm_api = {
    REDIS_GENERIC_QUAD_API(ocm)
    REDIS_GENERIC_STATS_API(ocm)
};
