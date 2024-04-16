#include "otai_vs.h"

VS_GENERIC_QUAD(ETHERNET,ethernet);
VS_GENERIC_STATS(ETHERNET,ethernet);

const otai_ethernet_api_t vs_ethernet_api = {
    VS_GENERIC_QUAD_API(ethernet)
    VS_GENERIC_STATS_API(ethernet)
};
