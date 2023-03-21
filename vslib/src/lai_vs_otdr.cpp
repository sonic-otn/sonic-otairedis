#include "lai_vs.h"

VS_GENERIC_QUAD(OTDR,otdr);
VS_GENERIC_STATS(OTDR,otdr);

const lai_otdr_api_t vs_otdr_api = {
    VS_GENERIC_QUAD_API(otdr)
    VS_GENERIC_STATS_API(otdr)
};
