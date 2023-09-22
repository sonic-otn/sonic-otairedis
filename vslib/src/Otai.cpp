#include "Otai.h"
#include "OtaiInternal.h"
#include "RealObjectIdManager.h"
#include "VirtualLinecardOtaiInterface.h"
#include "LinecardStateBase.h"
#include "LaneMapFileParser.h"
#include "LinecardConfigContainer.h"
#include "ResourceLimiterParser.h"
#include "CorePortIndexMapFileParser.h"

#include "swss/logger.h"

#include "swss/notificationconsumer.h"
#include "swss/select.h"

#include "otaivs.h"

#include <unistd.h>
#include <inttypes.h>

#include <algorithm>
#include <cstring>

using namespace otaivs;

#define VS_CHECK_API_INITIALIZED()                                          \
    if (!m_apiInitialized) {                                                \
        SWSS_LOG_ERROR("%s: api not initialized", __PRETTY_FUNCTION__);     \
        return OTAI_STATUS_FAILURE; }

extern otai_object_id_t g_scanning_ocm_oid;
extern bool g_ocm_scan;

extern otai_object_id_t g_scanning_otdr_oid;
extern bool g_otdr_scan;

Otai::Otai()
{
    SWSS_LOG_ENTER();

    m_unittestChannelRun = false;

    m_apiInitialized = false;

    m_isLinkUp = true;
    m_isAlarm = false;
    m_isEvent = false;
}

Otai::~Otai()
{
    SWSS_LOG_ENTER();

    if (m_apiInitialized)
    {
        uninitialize();
    }
}

// INITIALIZE UNINITIALIZE

otai_status_t Otai::initialize(
        _In_ uint64_t flags,
        _In_ const otai_service_method_table_t *service_method_table)
{
    MUTEX();

    SWSS_LOG_ENTER();

    if (m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: api already initialized", __PRETTY_FUNCTION__);

        return OTAI_STATUS_FAILURE;
    }

    if (flags != 0)
    {
        SWSS_LOG_ERROR("invalid flags passed to OTAI API initialize");

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if ((service_method_table == NULL) ||
            (service_method_table->profile_get_next_value == NULL) ||
            (service_method_table->profile_get_value == NULL))
    {
        SWSS_LOG_ERROR("invalid service_method_table handle passed to OTAI API initialize");

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    memcpy(&m_service_method_table, service_method_table, sizeof(m_service_method_table));

    auto linecard_type = service_method_table->profile_get_value(0, OTAI_KEY_VS_LINECARD_TYPE);

    if (linecard_type == NULL)
    {
        SWSS_LOG_ERROR("failed to obtain service method table value: %s", OTAI_KEY_VS_LINECARD_TYPE);

        return OTAI_STATUS_FAILURE;
    }

    auto linecard_location = service_method_table->profile_get_value(0, OTAI_KEY_VS_LINECARD_LOCATION);

    if (linecard_location == NULL)
    {
        SWSS_LOG_ERROR("failed to obtain service method table value: %s", OTAI_KEY_VS_LINECARD_LOCATION);

        return OTAI_STATUS_FAILURE;
    } else {
        SWSS_LOG_NOTICE("location = %s", linecard_location);
    }

    auto *resourceLimiterFile = service_method_table->profile_get_value(0, OTAI_KEY_VS_RESOURCE_LIMITER_FILE);

    m_resourceLimiterContainer = ResourceLimiterParser::parseFromFile(resourceLimiterFile);

    otai_vs_linecard_type_t linecardType;

    if (!LinecardConfig::parseLinecardType(linecard_type, linecardType))
    {
        return OTAI_STATUS_FAILURE;
    }

    m_signal = std::make_shared<Signal>();

    m_eventQueue = std::make_shared<EventQueue>(m_signal);

    auto sc = std::make_shared<LinecardConfig>();

    sc->m_linecardType = linecardType;
    sc->m_linecardIndex = 0;
    sc->m_eventQueue = m_eventQueue;
    sc->m_resourceLimiter = m_resourceLimiterContainer->getResourceLimiter(sc->m_linecardIndex);

    auto scc = std::make_shared<LinecardConfigContainer>();

    // TODO add support for multiple linecards, (global context?) and config context will need
    // to be passed over OTAI_KEY_ service method table, and here we need to load them
    // we also need global context value for those linecards (VirtualLinecardOtaiInterface/RealObjectIdManager)

    scc->insert(sc);

    // most important

    m_vsOtai = std::make_shared<VirtualLinecardOtaiInterface>(scc);

    m_meta = std::make_shared<otaimeta::Meta>(m_vsOtai);

    m_vsOtai->setMeta(m_meta);

    startEventQueueThread();

    startUnittestThread();

    startCheckLinkThread();

    m_apiInitialized = true;

    return OTAI_STATUS_SUCCESS;
}

otai_status_t Otai::uninitialize(void)
{
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    SWSS_LOG_NOTICE("begin");

    // no mutex on uninitialized to prevent deadlock
    // if some thread would try to gram api mutex
    // so threads must be stopped first

    SWSS_LOG_NOTICE("stopping threads");

    stopUnittestThread();

    stopEventQueueThread();

    stopCheckLinkThread();

    m_vsOtai = nullptr;
    m_meta = nullptr;

    m_apiInitialized = false;

    SWSS_LOG_NOTICE("end");

    return OTAI_STATUS_SUCCESS;
}

otai_status_t Otai::linkCheck(_Out_ bool *up)
{
    if (up == NULL) {
        return OTAI_STATUS_INVALID_PARAMETER;
    }
    *up = m_isLinkUp;
    return OTAI_STATUS_SUCCESS;
}

// QUAD OID

otai_status_t Otai::create(
        _In_ otai_object_type_t objectType,
        _Out_ otai_object_id_t* objectId,
        _In_ otai_object_id_t linecardId,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return m_meta->create(
            objectType,
            objectId,
            linecardId,
            attr_count,
            attr_list);
}

otai_status_t Otai::remove(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return m_meta->remove(objectType, objectId);
}
using namespace std;
otai_status_t Otai::set(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ const otai_attribute_t *attr)
{
    std::unique_lock<std::recursive_mutex> _lock(m_apimutex);
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    if (objectType == OTAI_OBJECT_TYPE_LINECARD)
    {
        if (attr)
        {
            if (attr->id == OTAI_VS_LINECARD_ATTR_META_ENABLE_UNITTESTS)
            {
                m_meta->meta_unittests_enable(attr->value.booldata);
                return OTAI_STATUS_SUCCESS;
            }

            if (attr->id == OTAI_VS_LINECARD_ATTR_META_ALLOW_READ_ONLY_ONCE)
            {
                return m_meta->meta_unittests_allow_readonly_set_once(OTAI_OBJECT_TYPE_LINECARD, attr->value.s32);
            }
        }
    }
    else if (objectType == OTAI_OBJECT_TYPE_OCM)
    {
        if (attr)
        {
            if (attr->id == OTAI_OCM_ATTR_SCAN &&
                attr->value.booldata == true)
            {
                g_scanning_ocm_oid = objectId;
                g_ocm_scan = true;
                return OTAI_STATUS_SUCCESS;
            }
        }
    }

    return m_meta->set(objectType, objectId, attr);
}

otai_status_t Otai::get(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ uint32_t attr_count,
        _Inout_ otai_attribute_t *attr_list)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return m_meta->get(
            objectType,
            objectId,
            attr_count,
            attr_list);
}

// QUAD SERIALIZED

otai_status_t Otai::create(
        _In_ otai_object_type_t object_type,
        _In_ const std::string& serializedObjectId,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_NOT_IMPLEMENTED;
}

otai_status_t Otai::remove(
        _In_ otai_object_type_t objectType,
        _In_ const std::string& serializedObjectId)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_NOT_IMPLEMENTED;
}

otai_status_t Otai::set(
        _In_ otai_object_type_t objectType,
        _In_ const std::string &serializedObjectId,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_NOT_IMPLEMENTED;
}

otai_status_t Otai::get(
        _In_ otai_object_type_t objectType,
        _In_ const std::string& serializedObjectId,
        _In_ uint32_t attr_count,
        _Inout_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_NOT_IMPLEMENTED;
}

// STATS

otai_status_t Otai::getStats(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids,
        _Out_ otai_stat_value_t *counters)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return m_meta->getStats(
            object_type,
            object_id,
            number_of_counters,
            counter_ids,
            counters);
}

otai_status_t Otai::getStatsExt(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids,
        _In_ otai_stats_mode_t mode,
        _Out_ otai_stat_value_t *counters)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return m_meta->getStatsExt(
            object_type,
            object_id,
            number_of_counters,
            counter_ids,
            mode,
            counters);
}

otai_status_t Otai::clearStats(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return m_meta->clearStats(
            object_type,
            object_id,
            number_of_counters,
            counter_ids);
}

// OTAI API

otai_status_t Otai::objectTypeGetAvailability(
        _In_ otai_object_id_t linecardId,
        _In_ otai_object_type_t objectType,
        _In_ uint32_t attrCount,
        _In_ const otai_attribute_t *attrList,
        _Out_ uint64_t *count)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return m_meta->objectTypeGetAvailability(
            linecardId,
            objectType,
            attrCount,
            attrList,
            count);
}

otai_status_t Otai::queryAttributeCapability(
        _In_ otai_object_id_t linecardId,
        _In_ otai_object_type_t objectType,
        _In_ otai_attr_id_t attrId,
        _Out_ otai_attr_capability_t *capability)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return m_meta->queryAttributeCapability(
            linecardId,
            objectType,
            attrId,
            capability);
}

otai_status_t Otai::queryAattributeEnumValuesCapability(
        _In_ otai_object_id_t linecard_id,
        _In_ otai_object_type_t object_type,
        _In_ otai_attr_id_t attr_id,
        _Inout_ otai_s32_list_t *enum_values_capability)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return m_meta->queryAattributeEnumValuesCapability(
            linecard_id,
            object_type,
            attr_id,
            enum_values_capability);
}

otai_object_type_t Otai::objectTypeQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (!m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: OTAI API not initialized", __PRETTY_FUNCTION__);

        return OTAI_OBJECT_TYPE_NULL;
    }

    // not need for metadata check or mutex since this method is static

    return RealObjectIdManager::objectTypeQuery(objectId);
}

otai_object_id_t Otai::linecardIdQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (!m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: OTAI API not initialized", __PRETTY_FUNCTION__);

        return OTAI_NULL_OBJECT_ID;
    }

    // not need for metadata check or mutex since this method is static

    return RealObjectIdManager::linecardIdQuery(objectId);
}

otai_status_t Otai::logSet(
        _In_ otai_api_t api,
        _In_ otai_log_level_t log_level)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return m_meta->logSet(api, log_level);
}
