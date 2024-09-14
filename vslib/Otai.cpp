#include "Otai.h"
#include "RealObjectIdManager.h"

#include "swss/logger.h"
#include "swss/notificationconsumer.h"
#include "swss/select.h"

#include "otaivs.h"

#include <unistd.h>
#include <inttypes.h>

#include <algorithm>
#include <cstring>

using namespace otaivs;
using namespace std;


#define VS_CHECK_API_INITIALIZED()                                          \
    if (!m_apiInitialized) {                                                \
        SWSS_LOG_ERROR("%s: api not initialized", __PRETTY_FUNCTION__);     \
        return OTAI_STATUS_FAILURE; }

#define MUTEX() std::lock_guard<std::recursive_mutex> _lock(m_apimutex)

Otai::Otai()
{
    SWSS_LOG_ENTER();
    m_apiInitialized = false;
    m_isLinkUp = true;
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

    auto linecard_type = service_method_table->profile_get_value(0, OTAI_KEY_VS_LINECARD_TYPE);
    if (linecard_type != NULL)
    {
        SWSS_LOG_NOTICE("linecard type = %s", linecard_type);
    }

    auto linecard_location = service_method_table->profile_get_value(0, OTAI_KEY_VS_LINECARD_LOCATION);
    if (linecard_location != NULL)
    {
        SWSS_LOG_NOTICE("location = %s", linecard_location);
    }

    m_apiInitialized = true;
    return OTAI_STATUS_SUCCESS;
}

otai_status_t Otai::uninitialize(void)
{
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    m_apiInitialized = false;
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

    auto otaiObjSim = OtaiObjectSimulator::getOtaiObjectSimulator(objectType);
    return otaiObjSim->create(
            objectId,
            linecardId,
            attr_count,
            attr_list);
}

otai_status_t Otai::remove(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    auto otaiObjSim = OtaiObjectSimulator::getOtaiObjectSimulator(object_type);
    return otaiObjSim->remove(object_id);
}
otai_status_t Otai::set(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ const otai_attribute_t *attr)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    auto otaiObjSim = OtaiObjectSimulator::getOtaiObjectSimulator(object_type);
    return otaiObjSim->set(object_id, attr);
}

otai_status_t Otai::get(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t objectId,
        _In_ uint32_t attr_count,
        _Inout_ otai_attribute_t *attr_list)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    auto otaiObjSim = OtaiObjectSimulator::getOtaiObjectSimulator(object_type);
    return otaiObjSim->get(
            objectId,
            attr_count,
            attr_list);
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

    auto otaiObjSim = OtaiObjectSimulator::getOtaiObjectSimulator(object_type);
    return otaiObjSim->getStatsExt(
            object_id,
            number_of_counters,
            counter_ids,
            OTAI_STATS_MODE_READ,
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

    auto otaiObjSim = OtaiObjectSimulator::getOtaiObjectSimulator(object_type);
    return otaiObjSim->getStatsExt(
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

    auto otaiObjSim = OtaiObjectSimulator::getOtaiObjectSimulator(object_type);
    return otaiObjSim->clearStats(
            object_id,
            number_of_counters,
            counter_ids);
}

// OTAI API
otai_object_type_t Otai::objectTypeQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (!m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: OTAI API not initialized", __PRETTY_FUNCTION__);
        return OTAI_OBJECT_TYPE_NULL;
    }

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

    return RealObjectIdManager::linecardIdQuery(objectId);
}

otai_status_t Otai::logSet(
        _In_ otai_api_t api,
        _In_ otai_log_level_t log_level)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VS_CHECK_API_INITIALIZED();

    return OTAI_STATUS_SUCCESS;
}
