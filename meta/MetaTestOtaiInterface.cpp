#include "MetaTestOtaiInterface.h"
#include "NumberOidIndexGenerator.h"
#include "LinecardConfigContainer.h"

#include "swss/logger.h"
#include "otai_serialize.h"

using namespace otaimeta;

MetaTestOtaiInterface::MetaTestOtaiInterface()
{
    SWSS_LOG_ENTER();

    auto sc = std::make_shared<otairedis::LinecardConfig>();

    sc->m_linecardIndex = 0;
    sc->m_hardwareInfo = "";

    auto scc = std::make_shared<otairedis::LinecardConfigContainer>();

    scc->insert(sc);

    m_virtualObjectIdManager = 
        std::make_shared<otairedis::VirtualObjectIdManager>(0, scc,
                std::make_shared<NumberOidIndexGenerator>());
}

static std::string getHardwareInfo(
        _In_ uint32_t attrCount,
        _In_ const otai_attribute_t *attrList)
{
    SWSS_LOG_ENTER();

    return "";
}

otai_status_t MetaTestOtaiInterface::create(
        _In_ otai_object_type_t objectType,
        _Out_ otai_object_id_t* objectId,
        _In_ otai_object_id_t linecardId,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (objectType == OTAI_OBJECT_TYPE_LINECARD)
    {
        // for given hardware info we always return same linecard id,
        // this is required since we could be performing warm boot here

        auto hwinfo = getHardwareInfo(attr_count, attr_list);

        linecardId = m_virtualObjectIdManager->allocateNewLinecardObjectId(hwinfo);

        *objectId = linecardId;

        if (linecardId == OTAI_NULL_OBJECT_ID)
        {
            SWSS_LOG_ERROR("linecard ID allocation failed");

            return OTAI_STATUS_FAILURE;
        }
    }
    else
    {
        *objectId = m_virtualObjectIdManager->allocateNewObjectId(objectType, linecardId);
    }

    if (*objectId == OTAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_ERROR("failed to allocated new object id: %s:%s",
                otai_serialize_object_type(objectType).c_str(),
                otai_serialize_object_id(linecardId).c_str());

        return OTAI_STATUS_FAILURE;
    }

    return OTAI_STATUS_SUCCESS;
}

otai_object_type_t MetaTestOtaiInterface::objectTypeQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_virtualObjectIdManager->otaiObjectTypeQuery(objectId);
}

otai_object_id_t MetaTestOtaiInterface::linecardIdQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_virtualObjectIdManager->otaiLinecardIdQuery(objectId);
}

