#include "VidManager.h"

#include "lib/VirtualObjectIdManager.h"

#include "meta/otai_serialize.h"

#include "swss/logger.h"

using namespace syncd;

otai_object_id_t VidManager::linecardIdQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (objectId == OTAI_NULL_OBJECT_ID)
    {
        return objectId;
    }

    auto swid = otairedis::VirtualObjectIdManager::linecardIdQuery(objectId);

    if (swid == OTAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("invalid object id %s",
                otai_serialize_object_id(objectId).c_str());
    }

    return swid;
}

otai_object_type_t VidManager::objectTypeQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (objectId == OTAI_NULL_OBJECT_ID)
    {
        return OTAI_OBJECT_TYPE_NULL;
    }

    otai_object_type_t ot = otairedis::VirtualObjectIdManager::objectTypeQuery(objectId);

    if (ot == OTAI_OBJECT_TYPE_NULL)
    {
        SWSS_LOG_THROW("invalid object id %s",
                otai_serialize_object_id(objectId).c_str());
    }

    return ot;
}

uint32_t VidManager::getLinecardIndex(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    otai_object_id_t swid = VidManager::linecardIdQuery(objectId);

    if (swid != OTAI_NULL_OBJECT_ID)
    {
        return otairedis::VirtualObjectIdManager::getLinecardIndex(swid);
    }

    SWSS_LOG_THROW("invalid obejct id: %s, should be LINECARD",
            otai_serialize_object_id(objectId).c_str());
}

uint32_t VidManager::getGlobalContext(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    auto swid = otairedis::VirtualObjectIdManager::linecardIdQuery(objectId);

    if (swid == OTAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("invalid object id %s",
                otai_serialize_object_id(objectId).c_str());
    }

    return otairedis::VirtualObjectIdManager::getGlobalContext(objectId);
}

uint64_t VidManager::getObjectIndex(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    auto swid = otairedis::VirtualObjectIdManager::linecardIdQuery(objectId);

    if (swid == OTAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("invalid object id %s",
                otai_serialize_object_id(objectId).c_str());
    }

    return otairedis::VirtualObjectIdManager::getObjectIndex(objectId);
}

otai_object_id_t VidManager::updateObjectIndex(
        _In_ otai_object_id_t objectId,
        _In_ uint64_t objectIndex)
{
    SWSS_LOG_ENTER();

    return otairedis::VirtualObjectIdManager::updateObjectIndex(objectId, objectIndex);
}
