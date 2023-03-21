#include "lai_vs.h"

VS_GENERIC_QUAD(APSPORT,apsport);
VS_GENERIC_STATS(APSPORT,apsport);

const lai_apsport_api_t vs_apsport_api = {
    VS_GENERIC_QUAD_API(apsport)
    VS_GENERIC_STATS_API(apsport)
};
