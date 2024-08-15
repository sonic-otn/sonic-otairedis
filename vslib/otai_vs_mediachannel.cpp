#include "otai_vs.h"

VS_GENERIC_QUAD(MEDIACHANNEL,mediachannel);
VS_GENERIC_STATS(MEDIACHANNEL,mediachannel);

const otai_mediachannel_api_t vs_mediachannel_api = {
    VS_GENERIC_QUAD_API(mediachannel)
    VS_GENERIC_STATS_API(mediachannel)
};
