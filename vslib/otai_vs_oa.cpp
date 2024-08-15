#include "otai_vs.h"

VS_GENERIC_QUAD(OA,oa);
VS_GENERIC_STATS(OA,oa);

const otai_oa_api_t vs_oa_api = {
    VS_GENERIC_QUAD_API(oa)
    VS_GENERIC_STATS_API(oa)
};
