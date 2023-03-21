#include "lai_vs.h"

VS_GENERIC_QUAD(ASSIGNMENT,assignment);
VS_GENERIC_STATS(ASSIGNMENT,assignment);

const lai_assignment_api_t vs_assignment_api = {

    VS_GENERIC_QUAD_API(assignment)
    VS_GENERIC_STATS_API(assignment)
};
