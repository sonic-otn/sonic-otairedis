#include "VirtualLinecardLaiInterface.h"

#include "../../lib/inc/PerformanceIntervalTimer.h"

#include "swss/logger.h"

#include "meta/lai_serialize.h"
#include "meta/LaiAttributeList.h"

#include <inttypes.h>

#include "meta/lai_serialize.h"

#include "LinecardStateBase.h"
#include "LinecardP230C.h"

/*
 * Max number of counters used in 1 api call
 */
#define VS_MAX_COUNTERS 128

#define MAX_HARDWARE_INFO_LENGTH 0x1000

using namespace laivs;
using namespace laimeta;
using namespace lairediscommon;

VirtualLinecardLaiInterface::VirtualLinecardLaiInterface(
        _In_ const std::shared_ptr<LinecardConfigContainer> scc)
{
    SWSS_LOG_ENTER();

    m_realObjectIdManager = std::make_shared<RealObjectIdManager>(0, scc); // TODO fix global context

    m_linecardConfigContainer = scc;
}

VirtualLinecardLaiInterface::~VirtualLinecardLaiInterface()
{
    SWSS_LOG_ENTER();

    // empty
}

lai_status_t VirtualLinecardLaiInterface::initialize(
        _In_ uint64_t flags,
        _In_ const lai_service_method_table_t *service_method_table)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_SUCCESS;
}

lai_status_t VirtualLinecardLaiInterface::uninitialize(void)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_SUCCESS;
}

lai_status_t VirtualLinecardLaiInterface::linkCheck(_Out_ bool *up)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_SUCCESS;
}

void VirtualLinecardLaiInterface::setMeta(
        _In_ std::weak_ptr<laimeta::Meta> meta)
{
    SWSS_LOG_ENTER();

    m_meta = meta;
}

std::string VirtualLinecardLaiInterface::getHardwareInfo(
        _In_ uint32_t attrCount,
        _In_ const lai_attribute_t *attrList) const
{
    SWSS_LOG_ENTER();

    return "";
}

lai_status_t VirtualLinecardLaiInterface::create(
        _In_ lai_object_type_t objectType,
        _Out_ lai_object_id_t* objectId,
        _In_ lai_object_id_t linecardId,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (!objectId)
    {
        SWSS_LOG_THROW("objectId pointer is NULL");
    }

    if (objectType == LAI_OBJECT_TYPE_LINECARD)
    {
        // for given hardware info we always return same linecard id,
        // this is required since we could be performing warm boot here

        auto hwinfo = getHardwareInfo(attr_count, attr_list);

        linecardId = m_realObjectIdManager->allocateNewLinecardObjectId(hwinfo);

        *objectId = linecardId;

        if (linecardId == LAI_NULL_OBJECT_ID)
        {
            SWSS_LOG_ERROR("linecard ID allocation failed");

            return LAI_STATUS_FAILURE;
        }
    }
    else
    {
        // create new real object ID
        *objectId = m_realObjectIdManager->allocateNewObjectId(objectType, linecardId);
    }

    std::string str_object_id = lai_serialize_object_id(*objectId);

    return create(
            linecardId,
            objectType,
            str_object_id,
            attr_count,
            attr_list);
}

lai_status_t VirtualLinecardLaiInterface::remove(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return remove(
            linecardIdQuery(objectId),
            objectType,
            lai_serialize_object_id(objectId));
}

lai_status_t VirtualLinecardLaiInterface::preSet(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_SUCCESS;
}

lai_status_t VirtualLinecardLaiInterface::set(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    auto status = preSet(objectType, objectId, attr);

    if (status != LAI_STATUS_SUCCESS)
        return status;

    return set(
            linecardIdQuery(objectId),
            objectType,
            lai_serialize_object_id(objectId),
            attr);
}

lai_status_t VirtualLinecardLaiInterface::get(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId,
        _In_ uint32_t attr_count,
        _Inout_ lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return get(
            linecardIdQuery(objectId),
            objectType,
            lai_serialize_object_id(objectId),
            attr_count,
            attr_list);
}

std::shared_ptr<LinecardStateBase> VirtualLinecardLaiInterface::init_linecard(
        _In_ lai_object_id_t linecard_id,
        _In_ std::shared_ptr<LinecardConfig> config,
        _In_ std::weak_ptr<laimeta::Meta> meta,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_TIMER("init");

    if (linecard_id == LAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("init linecard with NULL linecard id is not allowed");
    }

    if (m_linecardStateMap.find(linecard_id) != m_linecardStateMap.end())
    {
        SWSS_LOG_THROW("linecard already exists %s", lai_serialize_object_id(linecard_id).c_str());
    }
    SWSS_LOG_NOTICE("init_linecard type: %d", config->m_linecardType);
    switch (config->m_linecardType)
    {
        case LAI_VS_LINECARD_TYPE_P230C:

            m_linecardStateMap[linecard_id] = std::make_shared<LinecardP230C>(linecard_id, m_realObjectIdManager, config);
            break;

        default:

            SWSS_LOG_WARN("unknown linecard type: %d", config->m_linecardType);

            return nullptr;
    }

    auto ss = m_linecardStateMap.at(linecard_id);

    ss->setMeta(meta);

    lai_status_t status = ss->initialize_default_objects(attr_count, attr_list); // TODO move to constructor

    if (status != LAI_STATUS_SUCCESS)
    {
        SWSS_LOG_THROW("unable to init linecard %s", lai_serialize_status(status).c_str());
    }

    SWSS_LOG_NOTICE("initialized linecard %s", lai_serialize_object_id(linecard_id).c_str());

    return ss;
}

lai_status_t VirtualLinecardLaiInterface::create(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t object_type,
        _In_ const std::string& serializedObjectId,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (object_type == LAI_OBJECT_TYPE_LINECARD)
    {
        auto linecardIndex = RealObjectIdManager::getLinecardIndex(linecardId);

        auto config = m_linecardConfigContainer->getConfig(linecardIndex);

        if (config == nullptr)
        {
            SWSS_LOG_ERROR("failed to get linecard config for linecard %s, and index %u",
                    serializedObjectId.c_str(),
                    linecardIndex);

            return LAI_STATUS_FAILURE;
        }

        auto ss = init_linecard(linecardId, config, m_meta, attr_count, attr_list);

        if (!ss)
        {
            return LAI_STATUS_FAILURE;
        }
    }

    auto ss = m_linecardStateMap.at(linecardId);

    return ss->create(object_type, serializedObjectId, linecardId, attr_count, attr_list);
}

void VirtualLinecardLaiInterface::removeLinecard(
        _In_ lai_object_id_t linecard_id)
{
    SWSS_LOG_ENTER();

    if (m_linecardStateMap.find(linecard_id) == m_linecardStateMap.end())
    {
        SWSS_LOG_THROW("linecard doesn't exist 0x%" PRIx64, linecard_id);
    }

    SWSS_LOG_NOTICE("remove linecard 0x%" PRIx64, linecard_id);

    m_linecardStateMap.erase(linecard_id);
}

lai_status_t VirtualLinecardLaiInterface::remove(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ const std::string& serializedObjectId)
{
    SWSS_LOG_ENTER();

    auto ss = m_linecardStateMap.at(linecardId);

    auto status = ss->remove(objectType, serializedObjectId);

    if (objectType == LAI_OBJECT_TYPE_LINECARD &&
            status == LAI_STATUS_SUCCESS)
    {
        lai_object_id_t object_id;
        lai_deserialize_object_id(serializedObjectId, object_id);

        SWSS_LOG_NOTICE("removed linecard: %s", lai_serialize_object_id(object_id).c_str());

        m_realObjectIdManager->releaseObjectId(object_id);

        removeLinecard(object_id);
    }

    return status;
}

lai_status_t VirtualLinecardLaiInterface::set(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ const std::string &serializedObjectId,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    auto ss = m_linecardStateMap.at(linecardId);

    return ss->set(objectType, serializedObjectId, attr);
}

lai_status_t VirtualLinecardLaiInterface::get(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ const std::string& serializedObjectId,
        _In_ uint32_t attr_count,
        _Inout_ lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    auto ss = m_linecardStateMap.at(linecardId);
    return ss->get(objectType, serializedObjectId, attr_count, attr_list);
}

lai_status_t VirtualLinecardLaiInterface::objectTypeGetAvailability(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ uint32_t attrCount,
        _In_ const lai_attribute_t *attrList,
        _Out_ uint64_t *count)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_NOT_SUPPORTED;
}

lai_status_t VirtualLinecardLaiInterface::queryAttributeCapability(
        _In_ lai_object_id_t linecard_id,
        _In_ lai_object_type_t object_type,
        _In_ lai_attr_id_t attr_id,
        _Out_ lai_attr_capability_t *capability)
{
    SWSS_LOG_ENTER();

    // TODO: We should generate this metadata for the virtual linecard rather
    // than hard-coding it here.

    // in virtual linecard by default all apis are implemented for all objects. SUCCESS for all attributes

    capability->create_implemented = true;
    capability->set_implemented    = true;
    capability->get_implemented    = true;

    return LAI_STATUS_SUCCESS;
}

lai_status_t VirtualLinecardLaiInterface::queryAattributeEnumValuesCapability(
        _In_ lai_object_id_t linecard_id,
        _In_ lai_object_type_t object_type,
        _In_ lai_attr_id_t attr_id,
        _Inout_ lai_s32_list_t *enum_values_capability)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_NOT_SUPPORTED;
}

lai_status_t VirtualLinecardLaiInterface::getStats(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids,
        _Out_ lai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    /*
     * Get stats is the same as get stats ext with mode == LAI_STATS_MODE_READ.
     */

    return getStatsExt(
            object_type,
            object_id,
            number_of_counters,
            counter_ids,
            LAI_STATS_MODE_READ,
            counters);
}

lai_status_t VirtualLinecardLaiInterface::getStatsExt(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids,
        _In_ lai_stats_mode_t mode,
        _Out_ lai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    lai_object_id_t linecard_id = LAI_NULL_OBJECT_ID;

    if (m_linecardStateMap.size() == 0)
    {
        SWSS_LOG_ERROR("no switch!, was removed but some function still call");
        return LAI_STATUS_FAILURE;
    }

    if (m_linecardStateMap.size() == 1)
    {
        linecard_id = m_linecardStateMap.begin()->first;
    }
    else
    {
        SWSS_LOG_THROW("multiple switches not supported, FIXME");
    }

    if (m_linecardStateMap.find(linecard_id) == m_linecardStateMap.end())
    {
        SWSS_LOG_ERROR("failed to find switch %s in switch state map", lai_serialize_object_id(linecard_id).c_str());

        return LAI_STATUS_FAILURE;
    }

    auto ss = m_linecardStateMap.at(linecard_id);

    return ss->getStatsExt(
            object_type,
            object_id,
            number_of_counters,
            counter_ids,
            mode,
            counters);
}

lai_status_t VirtualLinecardLaiInterface::clearStats(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids)
{
    SWSS_LOG_ENTER();

    /*
     * Clear stats is the same as get stats ext with mode ==
     * LAI_STATS_MODE_READ_AND_CLEAR and we just read counters locally and
     * discard them, in that way.
     */

    lai_stat_value_t counters[VS_MAX_COUNTERS];

    return getStatsExt(
            object_type,
            object_id,
            number_of_counters,
            counter_ids,
            LAI_STATS_MODE_READ_AND_CLEAR,
            counters);
}

lai_status_t VirtualLinecardLaiInterface::getAlarms(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_alarms,
        _In_ const lai_alarm_type_t *alarm_ids,
        _Out_ lai_alarm_info_t *alarm_info)
{
    SWSS_LOG_ENTER();
    
    lai_object_id_t linecard_id = LAI_NULL_OBJECT_ID;

    if (m_linecardStateMap.size() == 0)
    {
        SWSS_LOG_ERROR("no switch!, was removed but some function still call");
        return LAI_STATUS_FAILURE;
    }

    if (m_linecardStateMap.size() == 1)
    {
        linecard_id = m_linecardStateMap.begin()->first;
    }
    else
    {
        SWSS_LOG_THROW("multiple linecards not supported, FIXME");
    }

    if (m_linecardStateMap.find(linecard_id) == m_linecardStateMap.end())
    {
        SWSS_LOG_ERROR("failed to find switch %s in switch state map", lai_serialize_object_id(linecard_id).c_str());

        return LAI_STATUS_FAILURE;
    }

    auto ss = m_linecardStateMap.at(linecard_id);

    return ss->getAlarms(
            object_type,
            object_id,
            number_of_alarms,
            alarm_ids,
            alarm_info);
}

lai_status_t VirtualLinecardLaiInterface::clearAlarms(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_alarms,
        _In_ const lai_alarm_type_t *alarm_ids)
{
    SWSS_LOG_ENTER();

    lai_object_id_t linecard_id = LAI_NULL_OBJECT_ID;

    if (m_linecardStateMap.size() == 0)
    {
        SWSS_LOG_ERROR("no switch!, was removed but some function still call");
        return LAI_STATUS_FAILURE;
    }

    if (m_linecardStateMap.size() == 1)
    {
        linecard_id = m_linecardStateMap.begin()->first;
    }
    else
    {
        SWSS_LOG_THROW("multiple linecards not supported, FIXME");
    }

    if (m_linecardStateMap.find(linecard_id) == m_linecardStateMap.end())
    {
        SWSS_LOG_ERROR("failed to find switch %s in switch state map", lai_serialize_object_id(linecard_id).c_str());

        return LAI_STATUS_FAILURE;
    }

    auto ss = m_linecardStateMap.at(linecard_id);

    return ss->clearAlarms(
            object_type,
            object_id,
            number_of_alarms,
            alarm_ids);
}

lai_object_type_t VirtualLinecardLaiInterface::objectTypeQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_realObjectIdManager->laiObjectTypeQuery(objectId);
}

lai_object_id_t VirtualLinecardLaiInterface::linecardIdQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_realObjectIdManager->laiLinecardIdQuery(objectId);
}

lai_status_t VirtualLinecardLaiInterface::logSet(
        _In_ lai_api_t api,
        _In_ lai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_SUCCESS;
}

void VirtualLinecardLaiInterface::syncProcessEventNetLinkMsg(
        _In_ std::shared_ptr<EventPayloadNetLinkMsg> payload)
{
    SWSS_LOG_ENTER();

    auto linecardId = payload->getLinecardId();

    auto it = m_linecardStateMap.find(linecardId);

    if (it == m_linecardStateMap.end())
    {
        SWSS_LOG_NOTICE("linecard %s don't exists in linecard state map",
                lai_serialize_object_id(linecardId).c_str());

        return;
    }
}
