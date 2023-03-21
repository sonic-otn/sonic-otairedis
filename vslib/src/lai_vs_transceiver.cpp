#include "lai_vs.h"

VS_GENERIC_QUAD(TRANSCEIVER,transceiver);
VS_GENERIC_STATS(TRANSCEIVER,transceiver);

const lai_transceiver_api_t vs_transceiver_api = {
    VS_GENERIC_QUAD_API(transceiver)
    VS_GENERIC_STATS_API(transceiver)
};
