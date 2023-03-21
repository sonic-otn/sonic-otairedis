#include "lai_vs.h"

VS_GENERIC_QUAD(LLDP,lldp);
VS_GENERIC_STATS(LLDP,lldp);

const lai_lldp_api_t vs_lldp_api = {

    VS_GENERIC_QUAD_API(lldp)
    VS_GENERIC_STATS_API(lldp)
};
