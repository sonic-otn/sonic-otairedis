#include "LaiInterface.h"

#include "swss/logger.h"

using namespace lairedis;

lai_status_t LaiInterface::create(
        _Inout_ lai_object_meta_key_t& metaKey,
        _In_ lai_object_id_t linecard_id,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    auto info = lai_metadata_get_object_type_info(metaKey.objecttype);

    if (!info)
    {
        SWSS_LOG_ERROR("invalid object type: %d", metaKey.objecttype);

        return LAI_STATUS_FAILURE;
    }

    if (info->isobjectid)
    {
        return create(metaKey.objecttype, &metaKey.objectkey.key.object_id, linecard_id, attr_count, attr_list);
    }

    SWSS_LOG_ERROR("object type %s not implemented, FIXME", info->objecttypename);

    return LAI_STATUS_FAILURE;
}

lai_status_t LaiInterface::remove(
        _In_ const lai_object_meta_key_t& metaKey)
{
    SWSS_LOG_ENTER();

    auto info = lai_metadata_get_object_type_info(metaKey.objecttype);

    if (!info)
    {
        SWSS_LOG_ERROR("invalid object type: %d", metaKey.objecttype);

        return LAI_STATUS_FAILURE;
    }

    if (info->isobjectid)
    {
        return remove(metaKey.objecttype, metaKey.objectkey.key.object_id);
    }

    SWSS_LOG_ERROR("object type %s not implemented, FIXME", info->objecttypename);

    return LAI_STATUS_FAILURE;
}

lai_status_t LaiInterface::set(
        _In_ const lai_object_meta_key_t& metaKey,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    auto info = lai_metadata_get_object_type_info(metaKey.objecttype);

    if (!info)
    {
        SWSS_LOG_ERROR("invalid object type: %d", metaKey.objecttype);

        return LAI_STATUS_FAILURE;
    }

    if (info->isobjectid)
    {
        return set(metaKey.objecttype, metaKey.objectkey.key.object_id, attr);
    }

    SWSS_LOG_ERROR("object type %s not implemented, FIXME", info->objecttypename);

    return LAI_STATUS_FAILURE;
}

lai_status_t LaiInterface::get(
        _In_ const lai_object_meta_key_t& metaKey,
        _In_ uint32_t attr_count,
        _Inout_ lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    auto info = lai_metadata_get_object_type_info(metaKey.objecttype);

    if (!info)
    {
        SWSS_LOG_ERROR("invalid object type: %d", metaKey.objecttype);

        return LAI_STATUS_FAILURE;
    }

    if (info->isobjectid)
    {
        return get(metaKey.objecttype, metaKey.objectkey.key.object_id, attr_count, attr_list);
    }

    SWSS_LOG_ERROR("object type %s not implemented, FIXME", info->objecttypename);

    return LAI_STATUS_FAILURE;
}
