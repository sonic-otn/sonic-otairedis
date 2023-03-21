#include "lai_redis.h"

REDIS_GENERIC_QUAD(APSPORT,apsport);
REDIS_GENERIC_STATS(APSPORT,apsport);

const lai_apsport_api_t redis_apsport_api = {
    REDIS_GENERIC_QUAD_API(apsport)
    REDIS_GENERIC_STATS_API(apsport)
};
