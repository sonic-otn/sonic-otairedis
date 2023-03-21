#include "lai_vs.h"

VS_GENERIC_QUAD(OCM,ocm);
VS_GENERIC_STATS(OCM,ocm);

const lai_ocm_api_t vs_ocm_api = {
    VS_GENERIC_QUAD_API(ocm)
    VS_GENERIC_STATS_API(ocm)
};
