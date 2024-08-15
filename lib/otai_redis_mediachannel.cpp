#include "otai_redis.h"

REDIS_GENERIC_QUAD(MEDIACHANNEL,mediachannel);
REDIS_GENERIC_STATS(MEDIACHANNEL,mediachannel);

const otai_mediachannel_api_t redis_mediachannel_api = {
    REDIS_GENERIC_QUAD_API(mediachannel)
    REDIS_GENERIC_STATS_API(mediachannel)
};
