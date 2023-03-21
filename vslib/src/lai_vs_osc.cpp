#include "lai_vs.h"

VS_GENERIC_QUAD(OSC,osc);
VS_GENERIC_STATS(OSC,osc);

const lai_osc_api_t vs_osc_api = {
    VS_GENERIC_QUAD_API(osc)
    VS_GENERIC_STATS_API(osc)
};
