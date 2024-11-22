#include "OtaiInterface.h"

#include "swss/logger.h"

using namespace otairedis;

otai_status_t OtaiInterface::create(
        _Inout_ otai_object_meta_key_t& metaKey,
        _In_ otai_object_id_t linecard_id,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return create(metaKey.objecttype, &metaKey.objectkey.key.object_id, linecard_id, attr_count, attr_list);
}

otai_status_t OtaiInterface::remove(
        _In_ const otai_object_meta_key_t& metaKey)
{
    SWSS_LOG_ENTER();

    return remove(metaKey.objecttype, metaKey.objectkey.key.object_id);
}

otai_status_t OtaiInterface::set(
        _In_ const otai_object_meta_key_t& metaKey,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    return set(metaKey.objecttype, metaKey.objectkey.key.object_id, attr);
}

otai_status_t OtaiInterface::get(
        _In_ const otai_object_meta_key_t& metaKey,
        _In_ uint32_t attr_count,
        _Inout_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return get(metaKey.objecttype, metaKey.objectkey.key.object_id, attr_count, attr_list);
}
