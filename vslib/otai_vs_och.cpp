#include "otai_vs.h"

VS_GENERIC_QUAD(OCH,och);
VS_GENERIC_STATS(OCH,och);

const otai_och_api_t vs_och_api =
{
    VS_GENERIC_QUAD_API(och)
    VS_GENERIC_STATS_API(och)
};
