#include "otai_vs.h"

VS_GENERIC_QUAD(APS,aps);
VS_GENERIC_STATS(APS,aps);

const otai_aps_api_t vs_aps_api = {
    VS_GENERIC_QUAD_API(aps)
    VS_GENERIC_STATS_API(aps)
};
