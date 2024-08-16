#include "otai_vs.h"

VS_GENERIC_QUAD(PHYSICALCHANNEL,physicalchannel);
VS_GENERIC_STATS(PHYSICALCHANNEL,physicalchannel);

const otai_physicalchannel_api_t vs_physicalchannel_api =
{
    VS_GENERIC_QUAD_API(physicalchannel)
    VS_GENERIC_STATS_API(physicalchannel)
};
