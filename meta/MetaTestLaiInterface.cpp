#include "MetaTestLaiInterface.h"
#include "NumberOidIndexGenerator.h"
#include "LinecardConfigContainer.h"

#include "swss/logger.h"
#include "lai_serialize.h"

using namespace laimeta;

MetaTestLaiInterface::MetaTestLaiInterface()
{
    SWSS_LOG_ENTER();

    auto sc = std::make_shared<lairedis::LinecardConfig>();

    sc->m_linecardIndex = 0;
    sc->m_hardwareInfo = "";

    auto scc = std::make_shared<lairedis::LinecardConfigContainer>();

    scc->insert(sc);

    m_virtualObjectIdManager = 
        std::make_shared<lairedis::VirtualObjectIdManager>(0, scc,
                std::make_shared<NumberOidIndexGenerator>());
}

static std::string getHardwareInfo(
        _In_ uint32_t attrCount,
        _In_ const lai_attribute_t *attrList)
{
    SWSS_LOG_ENTER();

    return "";
}

lai_status_t MetaTestLaiInterface::create(
        _In_ lai_object_type_t objectType,
        _Out_ lai_object_id_t* objectId,
        _In_ lai_object_id_t linecardId,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (objectType == LAI_OBJECT_TYPE_LINECARD)
    {
        // for given hardware info we always return same linecard id,
        // this is required since we could be performing warm boot here

        auto hwinfo = getHardwareInfo(attr_count, attr_list);

        linecardId = m_virtualObjectIdManager->allocateNewLinecardObjectId(hwinfo);

        *objectId = linecardId;

        if (linecardId == LAI_NULL_OBJECT_ID)
        {
            SWSS_LOG_ERROR("linecard ID allocation failed");

            return LAI_STATUS_FAILURE;
        }
    }
    else
    {
        *objectId = m_virtualObjectIdManager->allocateNewObjectId(objectType, linecardId);
    }

    if (*objectId == LAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_ERROR("failed to allocated new object id: %s:%s",
                lai_serialize_object_type(objectType).c_str(),
                lai_serialize_object_id(linecardId).c_str());

        return LAI_STATUS_FAILURE;
    }

    return LAI_STATUS_SUCCESS;
}

lai_object_type_t MetaTestLaiInterface::objectTypeQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_virtualObjectIdManager->laiObjectTypeQuery(objectId);
}

lai_object_id_t MetaTestLaiInterface::linecardIdQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_virtualObjectIdManager->laiLinecardIdQuery(objectId);
}

