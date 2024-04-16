#include "otai_vs.h"

VS_GENERIC_QUAD(WSS,wss);
VS_GENERIC_STATS(WSS,wss);

const otai_wss_api_t vs_wss_api = {
    VS_GENERIC_QUAD_API(wss)
    VS_GENERIC_STATS_API(wss)
};
