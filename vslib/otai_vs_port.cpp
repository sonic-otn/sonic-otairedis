#include "otai_vs.h"

VS_GENERIC_QUAD(PORT,port);
VS_GENERIC_STATS(PORT,port);

const otai_port_api_t vs_port_api = {

    VS_GENERIC_QUAD_API(port)
    VS_GENERIC_STATS_API(port)

};
