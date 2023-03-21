#include "VendorLai.h"

#include "meta/lai_serialize.h"

#include "swss/logger.h"

#include <cstring>

using namespace syncd;

#define MUTEX() std::lock_guard<std::mutex> _lock(m_apimutex)

#define VENDOR_CHECK_API_INITIALIZED()                                       \
    if (!m_apiInitialized) {                                                \
        SWSS_LOG_ERROR("%s: api not initialized", __PRETTY_FUNCTION__);     \
        return LAI_STATUS_FAILURE; }

VendorLai::VendorLai()
{
    SWSS_LOG_ENTER();

    m_apiInitialized = false;

    memset(&m_apis, 0, sizeof(m_apis));
}

VendorLai::~VendorLai()
{
    SWSS_LOG_ENTER();

    if (m_apiInitialized)
    {
        uninitialize();
    }
}

// INITIALIZE UNINITIALIZE

lai_status_t VendorLai::initialize(
    _In_ uint64_t flags,
    _In_ const lai_service_method_table_t* service_method_table)
{
    MUTEX();
    SWSS_LOG_ENTER();

    if (m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: api already initialized", __PRETTY_FUNCTION__);

        return LAI_STATUS_FAILURE;
    }

    if ((service_method_table == NULL))
    {
        SWSS_LOG_ERROR("invalid service_method_table handle passed to LAI API initialize");

        return LAI_STATUS_INVALID_PARAMETER;
    }

    memcpy(&m_service_method_table, service_method_table, sizeof(m_service_method_table));

    auto status = lai_api_initialize(flags, service_method_table);

    if (status == LAI_STATUS_SUCCESS)
    {
        memset(&m_apis, 0, sizeof(m_apis));

        int failed = lai_metadata_apis_query(lai_api_query, &m_apis);

        if (failed > 0)
        {
            SWSS_LOG_NOTICE("lai_api_query failed for %d apis", failed);
        }

        m_apiInitialized = true;
    }

    return status;
}

lai_status_t VendorLai::uninitialize(void)
{
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    auto status = lai_api_uninitialize();

    if (status == LAI_STATUS_SUCCESS)
    {
        m_apiInitialized = false;

        memset(&m_apis, 0, sizeof(m_apis));
    }

    return status;
}

lai_status_t VendorLai::linkCheck(_Out_ bool* up)
{
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    return lai_link_check(up);
}

// QUAD OID

lai_status_t VendorLai::create(
    _In_ lai_object_type_t objectType,
    _Out_ lai_object_id_t* objectId,
    _In_ lai_object_id_t linecardId,
    _In_ uint32_t attr_count,
    _In_ const lai_attribute_t* attr_list)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    auto info = lai_metadata_get_object_type_info(objectType);

    if (!info)
    {
        SWSS_LOG_ERROR("unable to get info for object type: %s",
            lai_serialize_object_type(objectType).c_str());

        return LAI_STATUS_FAILURE;
    }

    if (!info->create)
    {
        SWSS_LOG_ERROR("object type %s has no create method",
            lai_serialize_object_type(objectType).c_str());

        return LAI_STATUS_FAILURE;
    }

    if (info->isnonobjectid)
    {
        SWSS_LOG_ERROR("passed non object id as object id!: %s",
            lai_serialize_object_type(objectType).c_str());

        return LAI_STATUS_FAILURE;
    }

    lai_object_meta_key_t mk = { .objecttype = objectType, .objectkey = {.key = {.object_id = 0 } } };

    auto status = info->create(&mk, linecardId, attr_count, attr_list);

    if (status == LAI_STATUS_SUCCESS)
    {
        *objectId = mk.objectkey.key.object_id;
    }

    return status;
}

lai_status_t VendorLai::remove(
    _In_ lai_object_type_t objectType,
    _In_ lai_object_id_t objectId)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    auto info = lai_metadata_get_object_type_info(objectType);

    if (!info)
    {
        SWSS_LOG_ERROR("unable to get info for object type: %s",
            lai_serialize_object_type(objectType).c_str());

        return LAI_STATUS_FAILURE;
    }

    if (!info->remove)
    {
        SWSS_LOG_ERROR("object type %s has no remove method",
            lai_serialize_object_type(objectType).c_str());

        return LAI_STATUS_FAILURE;
    }

    if (info->isnonobjectid)
    {
        SWSS_LOG_ERROR("passed non object id as object id!: %s",
            lai_serialize_object_type(objectType).c_str());

        return LAI_STATUS_FAILURE;
    }

    lai_object_meta_key_t mk = { .objecttype = objectType, .objectkey = {.key = {.object_id = objectId } } };

    return info->remove(&mk);
}

lai_status_t VendorLai::set(
    _In_ lai_object_type_t objectType,
    _In_ lai_object_id_t objectId,
    _In_ const lai_attribute_t* attr)
{
    std::unique_lock<std::mutex> _lock(m_apimutex);
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    auto info = lai_metadata_get_object_type_info(objectType);

    if (!info)
    {
        SWSS_LOG_ERROR("unable to get info for object type: %s",
            lai_serialize_object_type(objectType).c_str());

        return LAI_STATUS_FAILURE;
    }

    if (!info->set)
    {
        SWSS_LOG_ERROR("object type %s has no set method",
            lai_serialize_object_type(objectType).c_str());

        return LAI_STATUS_FAILURE;
    }

    if (info->isnonobjectid)
    {
        SWSS_LOG_ERROR("passed non object id as object id!: %s",
            lai_serialize_object_type(objectType).c_str());

        return LAI_STATUS_FAILURE;
    }

    lai_object_meta_key_t mk = { .objecttype = objectType, .objectkey = {.key = {.object_id = objectId } } };

    return info->set(&mk, attr);
}

lai_status_t VendorLai::get(
    _In_ lai_object_type_t objectType,
    _In_ lai_object_id_t objectId,
    _In_ uint32_t attr_count,
    _Inout_ lai_attribute_t* attr_list)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    auto info = lai_metadata_get_object_type_info(objectType);

    if (!info)
    {
        SWSS_LOG_ERROR("unable to get info for object type: %s",
            lai_serialize_object_type(objectType).c_str());

        return LAI_STATUS_FAILURE;
    }

    if (!info->get)
    {
        SWSS_LOG_ERROR("object type %s has no get method",
            lai_serialize_object_type(objectType).c_str());

        return LAI_STATUS_FAILURE;
    }

    if (info->isnonobjectid)
    {
        SWSS_LOG_ERROR("passed non object id as object id!: %s",
            lai_serialize_object_type(objectType).c_str());

        return LAI_STATUS_FAILURE;
    }

    lai_object_meta_key_t mk = { .objecttype = objectType, .objectkey = {.key = {.object_id = objectId } } };

    return info->get(&mk, attr_count, attr_list);
}


// STATS

lai_status_t VendorLai::getStats(
    _In_ lai_object_type_t object_type,
    _In_ lai_object_id_t object_id,
    _In_ uint32_t number_of_counters,
    _In_ const lai_stat_id_t* counter_ids,
    _Out_ lai_stat_value_t* counters)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    lai_status_t(*ptr)(
        _In_ lai_object_id_t id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t * counter_ids,
        _Out_ lai_stat_value_t * counters) = nullptr;

    if (!counter_ids || !counters)
    {
        SWSS_LOG_ERROR("NULL pointer function argument");
        return LAI_STATUS_INVALID_PARAMETER;
    }

    switch ((int)object_type)
    {
    case LAI_OBJECT_TYPE_LINECARD:
        ptr = m_apis.linecard_api->get_linecard_stats;
        break;
    case LAI_OBJECT_TYPE_PORT:
        ptr = m_apis.port_api->get_port_stats;
        break;
    case LAI_OBJECT_TYPE_TRANSCEIVER:
        ptr = m_apis.transceiver_api->get_transceiver_stats;
        break;
    case LAI_OBJECT_TYPE_LOGICALCHANNEL:
        ptr = m_apis.logicalchannel_api->get_logicalchannel_stats;
        break;
    case LAI_OBJECT_TYPE_OTN:
        ptr = m_apis.otn_api->get_otn_stats;
        break;
    case LAI_OBJECT_TYPE_ETHERNET:
        ptr = m_apis.ethernet_api->get_ethernet_stats;
        break;
    case LAI_OBJECT_TYPE_PHYSICALCHANNEL:
        ptr = m_apis.physicalchannel_api->get_physicalchannel_stats;
        break;
    case LAI_OBJECT_TYPE_OCH:
        ptr = m_apis.och_api->get_och_stats;
        break;
    case LAI_OBJECT_TYPE_LLDP:
        ptr = m_apis.lldp_api->get_lldp_stats;
        break;
    case LAI_OBJECT_TYPE_ASSIGNMENT:
        ptr = m_apis.assignment_api->get_assignment_stats;
        break;
    case LAI_OBJECT_TYPE_INTERFACE:
        ptr = m_apis.interface_api->get_interface_stats;
        break;
    case LAI_OBJECT_TYPE_OA:
        ptr = m_apis.oa_api->get_oa_stats;
        break;
    case LAI_OBJECT_TYPE_OSC:
        ptr = m_apis.osc_api->get_osc_stats;
        break;
    case LAI_OBJECT_TYPE_APS:
        ptr = m_apis.aps_api->get_aps_stats;
        break;
    case LAI_OBJECT_TYPE_APSPORT:
        ptr = m_apis.apsport_api->get_apsport_stats;
        break;
    case LAI_OBJECT_TYPE_ATTENUATOR:
        ptr = m_apis.attenuator_api->get_attenuator_stats;
        break;
    default:
        SWSS_LOG_ERROR("not implemented, FIXME");
        return LAI_STATUS_FAILURE;
    }

    if (nullptr == ptr)
    {
        SWSS_LOG_ERROR("not implemented, FIXME");
        return LAI_STATUS_FAILURE;
    }

    return ptr(object_id, number_of_counters, counter_ids, counters);
}

lai_status_t VendorLai::getStatsExt(
    _In_ lai_object_type_t object_type,
    _In_ lai_object_id_t object_id,
    _In_ uint32_t number_of_counters,
    _In_ const lai_stat_id_t* counter_ids,
    _In_ lai_stats_mode_t mode,
    _Out_ lai_stat_value_t* counters)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    lai_status_t(*ptr)(
        _In_ lai_object_id_t id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t * counter_ids,
        _In_ lai_stats_mode_t mode,
        _Out_ lai_stat_value_t * counters) = nullptr;

    switch ((int)object_type)
    {
    default:
        SWSS_LOG_ERROR("not implemented, FIXME");
        return LAI_STATUS_FAILURE;
    }
    return ptr(object_id, number_of_counters, counter_ids, mode, counters);
}

lai_status_t VendorLai::clearStats(
    _In_ lai_object_type_t object_type,
    _In_ lai_object_id_t object_id,
    _In_ uint32_t number_of_counters,
    _In_ const lai_stat_id_t* counter_ids)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    lai_status_t(*ptr)(
        _In_ lai_object_id_t id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t * counter_ids) = nullptr;

    switch ((int)object_type)
    {
    case LAI_OBJECT_TYPE_LINECARD:
        ptr = m_apis.linecard_api->clear_linecard_stats;
        break;
    case LAI_OBJECT_TYPE_PORT:
        ptr = m_apis.port_api->clear_port_stats;
        break;
    case LAI_OBJECT_TYPE_TRANSCEIVER:
        ptr = m_apis.transceiver_api->clear_transceiver_stats;
        break;
    case LAI_OBJECT_TYPE_LOGICALCHANNEL:
        ptr = m_apis.logicalchannel_api->clear_logicalchannel_stats;
        break;
    case LAI_OBJECT_TYPE_OTN:
        ptr = m_apis.otn_api->clear_otn_stats;
        break;
    case LAI_OBJECT_TYPE_ETHERNET:
        ptr = m_apis.ethernet_api->clear_ethernet_stats;
        break;
    case LAI_OBJECT_TYPE_PHYSICALCHANNEL:
        ptr = m_apis.physicalchannel_api->clear_physicalchannel_stats;
        break;
    case LAI_OBJECT_TYPE_OCH:
        ptr = m_apis.och_api->clear_och_stats;
        break;
    case LAI_OBJECT_TYPE_LLDP:
        ptr = m_apis.lldp_api->clear_lldp_stats;
        break;
    case LAI_OBJECT_TYPE_ASSIGNMENT:
        ptr = m_apis.assignment_api->clear_assignment_stats;
        break;
    case LAI_OBJECT_TYPE_INTERFACE:
        ptr = m_apis.interface_api->clear_interface_stats;
        break;
    case LAI_OBJECT_TYPE_OA:
        ptr = m_apis.oa_api->clear_oa_stats;
        break;
    case LAI_OBJECT_TYPE_OSC:
        ptr = m_apis.osc_api->clear_osc_stats;
        break;
    case LAI_OBJECT_TYPE_APS:
        ptr = m_apis.aps_api->clear_aps_stats;
        break;
    case LAI_OBJECT_TYPE_APSPORT:
        ptr = m_apis.apsport_api->clear_apsport_stats;
        break;
    case LAI_OBJECT_TYPE_ATTENUATOR:
        ptr = m_apis.attenuator_api->clear_attenuator_stats;
        break;
    default:
        SWSS_LOG_ERROR("not implemented, FIXME");
        return LAI_STATUS_FAILURE;
    }

    if (nullptr == ptr)
    {
        SWSS_LOG_ERROR("not implemented, FIXME");
        return LAI_STATUS_FAILURE;
    }

    return ptr(object_id, number_of_counters, counter_ids);
}


lai_status_t VendorLai::getAlarms(
    _In_ lai_object_type_t object_type,
    _In_ lai_object_id_t object_id,
    _In_ uint32_t number_of_alarms,
    _In_ const lai_alarm_type_t* alarm_ids,
    _Out_ lai_alarm_info_t* alarm_info)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    lai_status_t(*ptr)(
        _In_ lai_object_id_t linecard_id,
        _In_ uint32_t number_of_alarms,
        _In_ const lai_alarm_type_t * alarm_ids,
        _Out_ lai_alarm_info_t * alarm_info) = nullptr;

    if (!alarm_ids || !alarm_info)
    {
        SWSS_LOG_ERROR("NULL pointer function argument");
        return LAI_STATUS_INVALID_PARAMETER;
    }

    switch ((int)object_type)
    {
    case LAI_OBJECT_TYPE_LINECARD:
        ptr = m_apis.linecard_api->get_linecard_alarms;
        break;
    default:
        SWSS_LOG_ERROR("not implemented, FIXME");
        return LAI_STATUS_FAILURE;
    }

    if (nullptr == ptr)
    {
        SWSS_LOG_ERROR("not implemented, FIXME");
        return LAI_STATUS_FAILURE;
    }
    return ptr(object_id, number_of_alarms, alarm_ids, alarm_info);
}

lai_status_t VendorLai::clearAlarms(
    _In_ lai_object_type_t object_type,
    _In_ lai_object_id_t object_id,
    _In_ uint32_t number_of_alarms,
    _In_ const lai_alarm_type_t* alarm_ids)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    lai_status_t(*ptr)(
        _In_ lai_object_id_t linecard_id,
        _In_ uint32_t number_of_alarms,
        _In_ const lai_alarm_type_t * alarm_ids) = nullptr;

    switch ((int)object_type)
    {
    case LAI_OBJECT_TYPE_LINECARD:
        ptr = m_apis.linecard_api->clear_linecard_alarms;
        break;

    default:
        SWSS_LOG_ERROR("not implemented, FIXME");
        return LAI_STATUS_FAILURE;
    }

    if (nullptr == ptr)
    {
        SWSS_LOG_ERROR("not implemented, FIXME");
        return LAI_STATUS_FAILURE;
    }

    return ptr(object_id, number_of_alarms, alarm_ids);
}

// LAI API

lai_status_t VendorLai::objectTypeGetAvailability(
    _In_ lai_object_id_t linecardId,
    _In_ lai_object_type_t objectType,
    _In_ uint32_t attrCount,
    _In_ const lai_attribute_t* attrList,
    _Out_ uint64_t* count)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    return lai_object_type_get_availability(
        linecardId,
        objectType,
        attrCount,
        attrList,
        count);
}

lai_status_t VendorLai::queryAttributeCapability(
    _In_ lai_object_id_t linecardId,
    _In_ lai_object_type_t objectType,
    _In_ lai_attr_id_t attrId,
    _Out_ lai_attr_capability_t* capability)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    return lai_query_attribute_capability(
        linecardId,
        objectType,
        attrId,
        capability);
}

lai_status_t VendorLai::queryAattributeEnumValuesCapability(
    _In_ lai_object_id_t linecardId,
    _In_ lai_object_type_t objectType,
    _In_ lai_attr_id_t attrId,
    _Inout_ lai_s32_list_t* enum_values_capability)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    return lai_query_attribute_enum_values_capability(
        linecardId,
        objectType,
        attrId,
        enum_values_capability);
}

lai_object_type_t VendorLai::objectTypeQuery(
    _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (!m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: LAI API not initialized", __PRETTY_FUNCTION__);

        return LAI_OBJECT_TYPE_NULL;
    }

    return lai_object_type_query(objectId);
}

lai_object_id_t VendorLai::linecardIdQuery(
    _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (!m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: LAI API not initialized", __PRETTY_FUNCTION__);

        return LAI_NULL_OBJECT_ID;
    }

    return lai_linecard_id_query(objectId);
}

lai_status_t VendorLai::logSet(
    _In_ lai_api_t api,
    _In_ lai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    return lai_log_set(api, log_level);
}
