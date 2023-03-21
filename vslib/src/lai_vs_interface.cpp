#include "lai_vs.h"

VS_GENERIC_QUAD(INTERFACE,interface);
VS_GENERIC_STATS(INTERFACE,interface);

const lai_interface_api_t vs_interface_api = {
    VS_GENERIC_QUAD_API(interface)
    VS_GENERIC_STATS_API(interface)
};
