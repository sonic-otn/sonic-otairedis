#include "lai_redis.h"

REDIS_GENERIC_QUAD(OA,oa);
REDIS_GENERIC_STATS(OA,oa);

const lai_oa_api_t redis_oa_api = {
    REDIS_GENERIC_QUAD_API(oa)
    REDIS_GENERIC_STATS_API(oa)
};
