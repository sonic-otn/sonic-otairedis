#include "otai_redis.h"

REDIS_GENERIC_QUAD(OCH,och);
REDIS_GENERIC_STATS(OCH,och);

const otai_och_api_t redis_och_api = {
    REDIS_GENERIC_QUAD_API(och)
    REDIS_GENERIC_STATS_API(och)
};
