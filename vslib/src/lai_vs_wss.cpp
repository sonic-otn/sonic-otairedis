#include "lai_vs.h"

VS_GENERIC_QUAD(WSS,wss);
VS_GENERIC_STATS(WSS,wss);

const lai_wss_api_t vs_wss_api = {
    VS_GENERIC_QUAD_API(wss)
    VS_GENERIC_STATS_API(wss)
};
