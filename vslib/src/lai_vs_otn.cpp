#include "lai_vs.h"

VS_GENERIC_QUAD(OTN,otn);
VS_GENERIC_STATS(OTN,otn);

const lai_otn_api_t vs_otn_api = {
    VS_GENERIC_QUAD_API(otn)
    VS_GENERIC_STATS_API(otn)
};
