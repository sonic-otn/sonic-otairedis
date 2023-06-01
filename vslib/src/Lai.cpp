#include "Lai.h"
#include "LaiInternal.h"
#include "RealObjectIdManager.h"
#include "VirtualLinecardLaiInterface.h"
#include "LinecardStateBase.h"
#include "LaneMapFileParser.h"
#include "LinecardConfigContainer.h"
#include "ResourceLimiterParser.h"
#include "CorePortIndexMapFileParser.h"

#include "swss/logger.h"

#include "swss/notificationconsumer.h"
#include "swss/select.h"

#include "laivs.h"

#include <unistd.h>
#include <inttypes.h>

#include <algorithm>
#include <cstring>

using namespace laivs;

#define VS_CHECK_API_INITIALIZED()                                          \
    if (!m_apiInitialized) {                                                \
        SWSS_LOG_ERROR("%s: api not initialized", __PRETTY_FUNCTION__);     \
        return LAI_STATUS_FAILURE; }

extern lai_object_id_t g_scanning_ocm_oid;
extern bool g_ocm_scan;

extern lai_object_id_t g_scanning_otdr_oid;
extern bool g_otdr_scan;

Lai::Lai()
{
    SWSS_LOG_ENTER();

    m_unittestChannelRun = false;

    m_apiInitialized = false;

    m_isLinkUp = true;
    m_isAlarm = false;
    m_isEvent = false;
}

Lai::~Lai()
{
    SWSS_LOG_ENTER();

    if (m_apiInitialized)
    {
        uninitialize();
    }
}

// INITIALIZE UNINITIALIZE

lai_status_t Lai::initialize(
        _In_ uint64_t flags,
        _In_ const lai_service_method_table_t *service_method_table)
{
    MUTEX();

    SWSS_LOG_ENTER();

    if (m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: api already initialized", __PRETTY_FUNCTION__);

        return LAI_STATUS_FAILURE;
    }

    if (flags != 0)
    {
        SWSS_LOG_ERROR("invalid flags passed to LAI API initialize");

        return LAI_STATUS_INVALID_PARAMETER;
    }

    if ((service_method_table == NULL) ||
            (service_method_table->profile_get_next_value == NULL) ||
            (service_method_table->profile_get_value == NULL))
    {
        SWSS_LOG_ERROR("invalid service_method_table handle passed to LAI API initialize");

        return LAI_STATUS_INVALID_PARAMETER;
    }

    memcpy(&m_service_method_table, service_method_table, sizeof(m_service_method_table));

    auto linecard_type = service_method_table->profile_get_value(0, LAI_KEY_VS_LINECARD_TYPE);

    if (linecard_type == NULL)
    {
        SWSS_LOG_ERROR("failed to obtain service method table value: %s", LAI_KEY_VS_LINECARD_TYPE);

        return LAI_STATUS_FAILURE;
    }

    auto linecard_location = service_method_table->profile_get_value(0, LAI_KEY_VS_LINECARD_LOCATION);

    if (linecard_location == NULL)
    {
        SWSS_LOG_ERROR("failed to obtain service method table value: %s", LAI_KEY_VS_LINECARD_LOCATION);

        return LAI_STATUS_FAILURE;
    } else {
        SWSS_LOG_NOTICE("location = %s", linecard_location);
    }

    auto *resourceLimiterFile = service_method_table->profile_get_value(0, LAI_KEY_VS_RESOURCE_LIMITER_FILE);

    m_resourceLimiterContainer = ResourceLimiterParser::parseFromFile(resourceLimiterFile);

    lai_vs_linecard_type_t linecardType;

    if (!LinecardConfig::parseLinecardType(linecard_type, linecardType))
    {
        return LAI_STATUS_FAILURE;
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
    // to be passed over LAI_KEY_ service method table, and here we need to load them
    // we also need global context value for those linecards (VirtualLinecardLaiInterface/RealObjectIdManager)

    scc->insert(sc);

    // most important

    m_vsLai = std::make_shared<VirtualLinecardLaiInterface>(scc);

    m_meta = std::make_shared<laimeta::Meta>(m_vsLai);

    m_vsLai->setMeta(m_meta);

    startEventQueueThread();

    startUnittestThread();

    startCheckLinkThread();

    m_apiInitialized = true;

    return LAI_STATUS_SUCCESS;
}

lai_status_t Lai::uninitialize(void)
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

    m_vsLai = nullptr;
    m_meta = nullptr;

    m_apiInitialized = false;

    SWSS_LOG_NOTICE("end");

    return LAI_STATUS_SUCCESS;
}

lai_status_t Lai::linkCheck(_Out_ bool *up)
{
    if (up == NULL) {
        return LAI_STATUS_INVALID_PARAMETER;
    }
    *up = m_isLinkUp;
    return LAI_STATUS_SUCCESS;
}

// QUAD OID

lai_status_t Lai::create(
        _In_ lai_object_type_t objectType,
        _Out_ lai_object_id_t* objectId,
        _In_ lai_object_id_t linecardId,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
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

lai_status_t Lai::remove(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return m_meta->remove(objectType, objectId);
}
using namespace std;
lai_status_t Lai::set(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId,
        _In_ const lai_attribute_t *attr)
{
    std::unique_lock<std::recursive_mutex> _lock(m_apimutex);
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    if (objectType == LAI_OBJECT_TYPE_LINECARD)
    {
        if (attr)
        {
            if (attr->id == LAI_VS_LINECARD_ATTR_META_ENABLE_UNITTESTS)
            {
                m_meta->meta_unittests_enable(attr->value.booldata);
                return LAI_STATUS_SUCCESS;
            }

            if (attr->id == LAI_VS_LINECARD_ATTR_META_ALLOW_READ_ONLY_ONCE)
            {
                return m_meta->meta_unittests_allow_readonly_set_once(LAI_OBJECT_TYPE_LINECARD, attr->value.s32);
            }
        }
    }
    else if (objectType == LAI_OBJECT_TYPE_OCM)
    {
        if (attr)
        {
            if (attr->id == LAI_OCM_ATTR_SCAN &&
                attr->value.booldata == true)
            {
                g_scanning_ocm_oid = objectId;
                g_ocm_scan = true;
                return LAI_STATUS_SUCCESS;
            }
        }
    }

    return m_meta->set(objectType, objectId, attr);
}

lai_status_t Lai::get(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId,
        _In_ uint32_t attr_count,
        _Inout_ lai_attribute_t *attr_list)
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

lai_status_t Lai::create(
        _In_ lai_object_type_t object_type,
        _In_ const std::string& serializedObjectId,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_NOT_IMPLEMENTED;
}

lai_status_t Lai::remove(
        _In_ lai_object_type_t objectType,
        _In_ const std::string& serializedObjectId)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_NOT_IMPLEMENTED;
}

lai_status_t Lai::set(
        _In_ lai_object_type_t objectType,
        _In_ const std::string &serializedObjectId,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_NOT_IMPLEMENTED;
}

lai_status_t Lai::get(
        _In_ lai_object_type_t objectType,
        _In_ const std::string& serializedObjectId,
        _In_ uint32_t attr_count,
        _Inout_ lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_NOT_IMPLEMENTED;
}

// STATS

lai_status_t Lai::getStats(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids,
        _Out_ lai_stat_value_t *counters)
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

lai_status_t Lai::getStatsExt(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids,
        _In_ lai_stats_mode_t mode,
        _Out_ lai_stat_value_t *counters)
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

lai_status_t Lai::clearStats(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids)
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

// LAI API

lai_status_t Lai::objectTypeGetAvailability(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ uint32_t attrCount,
        _In_ const lai_attribute_t *attrList,
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

lai_status_t Lai::queryAttributeCapability(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ lai_attr_id_t attrId,
        _Out_ lai_attr_capability_t *capability)
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

lai_status_t Lai::queryAattributeEnumValuesCapability(
        _In_ lai_object_id_t linecard_id,
        _In_ lai_object_type_t object_type,
        _In_ lai_attr_id_t attr_id,
        _Inout_ lai_s32_list_t *enum_values_capability)
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

lai_object_type_t Lai::objectTypeQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (!m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: LAI API not initialized", __PRETTY_FUNCTION__);

        return LAI_OBJECT_TYPE_NULL;
    }

    // not need for metadata check or mutex since this method is static

    return RealObjectIdManager::objectTypeQuery(objectId);
}

lai_object_id_t Lai::linecardIdQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (!m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: LAI API not initialized", __PRETTY_FUNCTION__);

        return LAI_NULL_OBJECT_ID;
    }

    // not need for metadata check or mutex since this method is static

    return RealObjectIdManager::linecardIdQuery(objectId);
}

lai_status_t Lai::logSet(
        _In_ lai_api_t api,
        _In_ lai_log_level_t log_level)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return m_meta->logSet(api, log_level);
}
