#include "VidManager.h"

#include "lib/inc/VirtualObjectIdManager.h"

#include "meta/lai_serialize.h"

#include "swss/logger.h"

using namespace syncd;

lai_object_id_t VidManager::linecardIdQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (objectId == LAI_NULL_OBJECT_ID)
    {
        return objectId;
    }

    auto swid = lairedis::VirtualObjectIdManager::linecardIdQuery(objectId);

    if (swid == LAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("invalid object id %s",
                lai_serialize_object_id(objectId).c_str());
    }

    return swid;
}

lai_object_type_t VidManager::objectTypeQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (objectId == LAI_NULL_OBJECT_ID)
    {
        return LAI_OBJECT_TYPE_NULL;
    }

    lai_object_type_t ot = lairedis::VirtualObjectIdManager::objectTypeQuery(objectId);

    if (ot == LAI_OBJECT_TYPE_NULL)
    {
        SWSS_LOG_THROW("invalid object id %s",
                lai_serialize_object_id(objectId).c_str());
    }

    return ot;
}

uint32_t VidManager::getLinecardIndex(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    lai_object_id_t swid = VidManager::linecardIdQuery(objectId);

    if (swid != LAI_NULL_OBJECT_ID)
    {
        return lairedis::VirtualObjectIdManager::getLinecardIndex(swid);
    }

    SWSS_LOG_THROW("invalid obejct id: %s, should be LINECARD",
            lai_serialize_object_id(objectId).c_str());
}

uint32_t VidManager::getGlobalContext(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    auto swid = lairedis::VirtualObjectIdManager::linecardIdQuery(objectId);

    if (swid == LAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("invalid object id %s",
                lai_serialize_object_id(objectId).c_str());
    }

    return lairedis::VirtualObjectIdManager::getGlobalContext(objectId);
}

uint64_t VidManager::getObjectIndex(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    auto swid = lairedis::VirtualObjectIdManager::linecardIdQuery(objectId);

    if (swid == LAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("invalid object id %s",
                lai_serialize_object_id(objectId).c_str());
    }

    return lairedis::VirtualObjectIdManager::getObjectIndex(objectId);
}

lai_object_id_t VidManager::updateObjectIndex(
        _In_ lai_object_id_t objectId,
        _In_ uint64_t objectIndex)
{
    SWSS_LOG_ENTER();

    return lairedis::VirtualObjectIdManager::updateObjectIndex(objectId, objectIndex);
}
