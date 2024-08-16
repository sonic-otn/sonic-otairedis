#include "VirtualLinecardOtaiInterface.h"


#include "swss/logger.h"

#include "meta/otai_serialize.h"
#include "meta/OtaiAttributeList.h"
#include "meta/PerformanceIntervalTimer.h"

#include <inttypes.h>

#include "meta/otai_serialize.h"

#include "LinecardStateBase.h"
#include "LinecardOTN.h"

/*
 * Max number of counters used in 1 api call
 */
#define VS_MAX_COUNTERS 128

#define MAX_HARDWARE_INFO_LENGTH 0x1000

using namespace otaivs;
using namespace otaimeta;
using namespace otairediscommon;

VirtualLinecardOtaiInterface::VirtualLinecardOtaiInterface(
        _In_ const std::shared_ptr<LinecardConfigContainer> scc)
{
    SWSS_LOG_ENTER();

    m_realObjectIdManager = std::make_shared<RealObjectIdManager>(0, scc); // TODO fix global context

    m_linecardConfigContainer = scc;
}

VirtualLinecardOtaiInterface::~VirtualLinecardOtaiInterface()
{
    SWSS_LOG_ENTER();

    // empty
}

otai_status_t VirtualLinecardOtaiInterface::initialize(
        _In_ uint64_t flags,
        _In_ const otai_service_method_table_t *service_method_table)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_SUCCESS;
}

otai_status_t VirtualLinecardOtaiInterface::uninitialize(void)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_SUCCESS;
}

otai_status_t VirtualLinecardOtaiInterface::linkCheck(_Out_ bool *up)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_SUCCESS;
}

void VirtualLinecardOtaiInterface::setMeta(
        _In_ std::weak_ptr<otaimeta::Meta> meta)
{
    SWSS_LOG_ENTER();

    m_meta = meta;
}

std::string VirtualLinecardOtaiInterface::getHardwareInfo(
        _In_ uint32_t attrCount,
        _In_ const otai_attribute_t *attrList) const
{
    SWSS_LOG_ENTER();

    return "";
}

otai_status_t VirtualLinecardOtaiInterface::create(
        _In_ otai_object_type_t objectType,
        _Out_ otai_object_id_t* objectId,
        _In_ otai_object_id_t linecardId,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (!objectId)
    {
        SWSS_LOG_THROW("objectId pointer is NULL");
    }

    if (objectType == OTAI_OBJECT_TYPE_LINECARD)
    {
        // for given hardware info we always return same linecard id,
        // this is required since we could be performing warm boot here

        auto hwinfo = getHardwareInfo(attr_count, attr_list);

        linecardId = m_realObjectIdManager->allocateNewLinecardObjectId(hwinfo);

        *objectId = linecardId;

        if (linecardId == OTAI_NULL_OBJECT_ID)
        {
            SWSS_LOG_ERROR("linecard ID allocation failed");

            return OTAI_STATUS_FAILURE;
        }
    }
    else
    {
        // create new real object ID
        *objectId = m_realObjectIdManager->allocateNewObjectId(objectType, linecardId, attr_count, attr_list);
    }

    std::string str_object_id = otai_serialize_object_id(*objectId);

    return create(
            linecardId,
            objectType,
            str_object_id,
            attr_count,
            attr_list);
}

otai_status_t VirtualLinecardOtaiInterface::remove(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return remove(
            linecardIdQuery(objectId),
            objectType,
            otai_serialize_object_id(objectId));
}

otai_status_t VirtualLinecardOtaiInterface::preSet(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_SUCCESS;
}

otai_status_t VirtualLinecardOtaiInterface::set(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    auto status = preSet(objectType, objectId, attr);

    if (status != OTAI_STATUS_SUCCESS)
        return status;

    return set(
            linecardIdQuery(objectId),
            objectType,
            otai_serialize_object_id(objectId),
            attr);
}

otai_status_t VirtualLinecardOtaiInterface::get(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ uint32_t attr_count,
        _Inout_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return get(
            linecardIdQuery(objectId),
            objectType,
            otai_serialize_object_id(objectId),
            attr_count,
            attr_list);
}

std::shared_ptr<LinecardStateBase> VirtualLinecardOtaiInterface::init_linecard(
        _In_ otai_object_id_t linecard_id,
        _In_ std::shared_ptr<LinecardConfig> config,
        _In_ std::weak_ptr<otaimeta::Meta> meta,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_TIMER("init");

    if (linecard_id == OTAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("init linecard with NULL linecard id is not allowed");
    }

    if (m_linecardStateMap.find(linecard_id) == m_linecardStateMap.end())
    {
        SWSS_LOG_NOTICE("init_linecard type: %d", config->m_linecardType);
        switch (config->m_linecardType)
        {
            case OTAI_VS_LINECARD_TYPE_OTN:

                m_linecardStateMap[linecard_id] = std::make_shared<LinecardOTN>(linecard_id, m_realObjectIdManager, config);
                break;

            default:

                SWSS_LOG_WARN("unknown linecard type: %d", config->m_linecardType);

                return nullptr;
        }
    }

    auto ss = m_linecardStateMap.at(linecard_id);

    ss->setMeta(meta);

    otai_status_t status = ss->initialize_default_objects(attr_count, attr_list); // TODO move to constructor

    if (status != OTAI_STATUS_SUCCESS)
    {
        SWSS_LOG_THROW("unable to init linecard %s", otai_serialize_status(status).c_str());
    }

    SWSS_LOG_NOTICE("initialized linecard %s", otai_serialize_object_id(linecard_id).c_str());

    return ss;
}

otai_status_t VirtualLinecardOtaiInterface::create(
        _In_ otai_object_id_t linecardId,
        _In_ otai_object_type_t object_type,
        _In_ const std::string& serializedObjectId,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (object_type == OTAI_OBJECT_TYPE_LINECARD)
    {
        auto linecardIndex = RealObjectIdManager::getLinecardIndex(linecardId);

        auto config = m_linecardConfigContainer->getConfig(linecardIndex);

        if (config == nullptr)
        {
            SWSS_LOG_ERROR("failed to get linecard config for linecard %s, and index %u",
                    serializedObjectId.c_str(),
                    linecardIndex);

            return OTAI_STATUS_FAILURE;
        }

        auto ss = init_linecard(linecardId, config, m_meta, attr_count, attr_list);

        if (!ss)
        {
            return OTAI_STATUS_FAILURE;
        }
    }

    auto ss = m_linecardStateMap.at(linecardId);

    return ss->create(object_type, serializedObjectId, linecardId, attr_count, attr_list);
}

void VirtualLinecardOtaiInterface::removeLinecard(
        _In_ otai_object_id_t linecard_id)
{
    SWSS_LOG_ENTER();

    if (m_linecardStateMap.find(linecard_id) == m_linecardStateMap.end())
    {
        SWSS_LOG_THROW("linecard doesn't exist 0x%" PRIx64, linecard_id);
    }

    SWSS_LOG_NOTICE("remove linecard 0x%" PRIx64, linecard_id);

    m_linecardStateMap.erase(linecard_id);
}

otai_status_t VirtualLinecardOtaiInterface::remove(
        _In_ otai_object_id_t linecardId,
        _In_ otai_object_type_t objectType,
        _In_ const std::string& serializedObjectId)
{
    SWSS_LOG_ENTER();

    auto ss = m_linecardStateMap.at(linecardId);

    auto status = ss->remove(objectType, serializedObjectId);

    if (objectType == OTAI_OBJECT_TYPE_LINECARD &&
            status == OTAI_STATUS_SUCCESS)
    {
        otai_object_id_t object_id;
        otai_deserialize_object_id(serializedObjectId, object_id);

        SWSS_LOG_NOTICE("removed linecard: %s", otai_serialize_object_id(object_id).c_str());

        m_realObjectIdManager->releaseObjectId(object_id);

        removeLinecard(object_id);
    }

    return status;
}

otai_status_t VirtualLinecardOtaiInterface::set(
        _In_ otai_object_id_t linecardId,
        _In_ otai_object_type_t objectType,
        _In_ const std::string &serializedObjectId,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    auto ss = m_linecardStateMap.at(linecardId);

    return ss->set(objectType, serializedObjectId, attr);
}

otai_status_t VirtualLinecardOtaiInterface::get(
        _In_ otai_object_id_t linecardId,
        _In_ otai_object_type_t objectType,
        _In_ const std::string& serializedObjectId,
        _In_ uint32_t attr_count,
        _Inout_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    auto ss = m_linecardStateMap.at(linecardId);
    return ss->get(objectType, serializedObjectId, attr_count, attr_list);
}

otai_status_t VirtualLinecardOtaiInterface::getStats(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids,
        _Out_ otai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    /*
     * Get stats is the same as get stats ext with mode == OTAI_STATS_MODE_READ.
     */

    return getStatsExt(
            object_type,
            object_id,
            number_of_counters,
            counter_ids,
            OTAI_STATS_MODE_READ,
            counters);
}

otai_status_t VirtualLinecardOtaiInterface::getStatsExt(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids,
        _In_ otai_stats_mode_t mode,
        _Out_ otai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    otai_object_id_t linecard_id = OTAI_NULL_OBJECT_ID;

    if (m_linecardStateMap.size() == 0)
    {
        SWSS_LOG_ERROR("no switch!, was removed but some function still call");
        return OTAI_STATUS_FAILURE;
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
        SWSS_LOG_ERROR("failed to find switch %s in switch state map", otai_serialize_object_id(linecard_id).c_str());

        return OTAI_STATUS_FAILURE;
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

otai_status_t VirtualLinecardOtaiInterface::clearStats(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids)
{
    SWSS_LOG_ENTER();

    /*
     * Clear stats is the same as get stats ext with mode ==
     * OTAI_STATS_MODE_READ_AND_CLEAR and we just read counters locally and
     * discard them, in that way.
     */

    otai_stat_value_t counters[VS_MAX_COUNTERS];

    return getStatsExt(
            object_type,
            object_id,
            number_of_counters,
            counter_ids,
            OTAI_STATS_MODE_READ_AND_CLEAR,
            counters);
}

otai_object_type_t VirtualLinecardOtaiInterface::objectTypeQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_realObjectIdManager->otaiObjectTypeQuery(objectId);
}

otai_object_id_t VirtualLinecardOtaiInterface::linecardIdQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_realObjectIdManager->otaiLinecardIdQuery(objectId);
}

otai_status_t VirtualLinecardOtaiInterface::logSet(
        _In_ otai_api_t api,
        _In_ otai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_SUCCESS;
}

