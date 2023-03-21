#include "lai_redis.h"

REDIS_GENERIC_QUAD(ASSIGNMENT,assignment);
REDIS_GENERIC_STATS(ASSIGNMENT,assignment);

const lai_assignment_api_t redis_assignment_api = {
    REDIS_GENERIC_QUAD_API(assignment)
    REDIS_GENERIC_STATS_API(assignment)
};
