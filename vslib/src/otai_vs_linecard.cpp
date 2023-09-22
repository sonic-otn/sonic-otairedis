#include "otai_vs.h"

VS_GENERIC_QUAD(LINECARD,linecard);
VS_GENERIC_STATS(LINECARD,linecard);

static otai_status_t vs_create_linecard_uniq(
        _Out_ otai_object_id_t *linecard_id,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return vs_create_linecard(
            linecard_id,
            OTAI_NULL_OBJECT_ID, // no linecard id since we create linecard
            attr_count,
            attr_list);
}

const otai_linecard_api_t vs_linecard_api = {

    vs_create_linecard_uniq,
    vs_remove_linecard,
    vs_set_linecard_attribute,
    vs_get_linecard_attribute,
    VS_GENERIC_STATS_API(linecard)
};
