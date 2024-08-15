#include "otai_redis.h"

REDIS_GENERIC_QUAD(OTN,otn);
REDIS_GENERIC_STATS(OTN,otn);

const otai_otn_api_t redis_otn_api = {
    REDIS_GENERIC_QUAD_API(otn)
    REDIS_GENERIC_STATS_API(otn)
};
