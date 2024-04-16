#include "otai_vs.h"

VS_GENERIC_QUAD(ATTENUATOR,attenuator);
VS_GENERIC_STATS(ATTENUATOR,attenuator);

const otai_attenuator_api_t vs_attenuator_api = {
    VS_GENERIC_QUAD_API(attenuator)
    VS_GENERIC_STATS_API(attenuator)
};
