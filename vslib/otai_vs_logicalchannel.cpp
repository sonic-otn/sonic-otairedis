#include "otai_vs.h"

VS_GENERIC_QUAD(LOGICALCHANNEL,logicalchannel);
VS_GENERIC_STATS(LOGICALCHANNEL,logicalchannel);

const otai_logicalchannel_api_t vs_logicalchannel_api = 
{
    VS_GENERIC_QUAD_API(logicalchannel)
    VS_GENERIC_STATS_API(logicalchannel)
};
