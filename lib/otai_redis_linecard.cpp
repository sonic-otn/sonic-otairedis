#include "otai_redis.h"

REDIS_GENERIC_QUAD(LINECARD,linecard);
REDIS_GENERIC_STATS(LINECARD,linecard);

static otai_status_t redis_create_linecard_uniq(
        _Out_ otai_object_id_t *linecard_id,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return redis_create_linecard(
            linecard_id,
            OTAI_NULL_OBJECT_ID, // no linecard id since we create linecard
            attr_count,
            attr_list);
}

const otai_linecard_api_t redis_linecard_api = {
    redis_create_linecard_uniq,
    redis_remove_linecard,
    redis_set_linecard_attribute,
    redis_get_linecard_attribute,
    REDIS_GENERIC_STATS_API(linecard)
};
