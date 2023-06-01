#include "lai_vs.h"

VS_GENERIC_QUAD(LINECARD,linecard);
VS_GENERIC_STATS(LINECARD,linecard);

static lai_status_t vs_create_linecard_uniq(
        _Out_ lai_object_id_t *linecard_id,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return vs_create_linecard(
            linecard_id,
            LAI_NULL_OBJECT_ID, // no linecard id since we create linecard
            attr_count,
            attr_list);
}

const lai_linecard_api_t vs_linecard_api = {

    vs_create_linecard_uniq,
    vs_remove_linecard,
    vs_set_linecard_attribute,
    vs_get_linecard_attribute,
    VS_GENERIC_STATS_API(linecard)
};
