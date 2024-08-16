#include "VendorOtai.h"

#include "meta/otai_serialize.h"

#include "swss/logger.h"

#include <cstring>

using namespace syncd;

#define MUTEX() std::lock_guard<std::mutex> _lock(m_apimutex)

#define VENDOR_CHECK_API_INITIALIZED()                                       \
    if (!m_apiInitialized) {                                                \
        SWSS_LOG_ERROR("%s: api not initialized", __PRETTY_FUNCTION__);     \
        return OTAI_STATUS_FAILURE; }

VendorOtai::VendorOtai()
{
    SWSS_LOG_ENTER();

    m_apiInitialized = false;

    memset(&m_apis, 0, sizeof(m_apis));
}

VendorOtai::~VendorOtai()
{
    SWSS_LOG_ENTER();

    if (m_apiInitialized)
    {
        uninitialize();
    }
}

// INITIALIZE UNINITIALIZE

otai_status_t VendorOtai::initialize(
    _In_ uint64_t flags,
    _In_ const otai_service_method_table_t* service_method_table)
{
    MUTEX();
    SWSS_LOG_ENTER();

    if (m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: api already initialized", __PRETTY_FUNCTION__);

        return OTAI_STATUS_FAILURE;
    }

    if ((service_method_table == NULL))
    {
        SWSS_LOG_ERROR("invalid service_method_table handle passed to OTAI API initialize");

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    memcpy(&m_service_method_table, service_method_table, sizeof(m_service_method_table));

    auto status = otai_api_initialize(flags, service_method_table);

    if (status == OTAI_STATUS_SUCCESS)
    {
        memset(&m_apis, 0, sizeof(m_apis));

        int failed = otai_metadata_apis_query(otai_api_query, &m_apis);

        if (failed > 0)
        {
            SWSS_LOG_NOTICE("otai_api_query failed for %d apis", failed);
        }

        m_apiInitialized = true;
    }

    return status;
}

otai_status_t VendorOtai::uninitialize(void)
{
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    auto status = otai_api_uninitialize();

    if (status == OTAI_STATUS_SUCCESS)
    {
        m_apiInitialized = false;

        memset(&m_apis, 0, sizeof(m_apis));
    }

    return status;
}

otai_status_t VendorOtai::linkCheck(_Out_ bool* up)
{
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    return otai_link_check(up);
}

// QUAD OID

otai_status_t VendorOtai::create(
    _In_ otai_object_type_t objectType,
    _Out_ otai_object_id_t* objectId,
    _In_ otai_object_id_t linecardId,
    _In_ uint32_t attr_count,
    _In_ const otai_attribute_t* attr_list)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    auto info = otai_metadata_get_object_type_info(objectType);

    if (!info)
    {
        SWSS_LOG_ERROR("unable to get info for object type: %s",
            otai_serialize_object_type(objectType).c_str());

        return OTAI_STATUS_FAILURE;
    }

    if (!info->create)
    {
        SWSS_LOG_ERROR("object type %s has no create method",
            otai_serialize_object_type(objectType).c_str());

        return OTAI_STATUS_FAILURE;
    }

    if (info->isnonobjectid)
    {
        SWSS_LOG_ERROR("passed non object id as object id!: %s",
            otai_serialize_object_type(objectType).c_str());

        return OTAI_STATUS_FAILURE;
    }

    otai_object_meta_key_t mk = { .objecttype = objectType, .objectkey = {.key = {.object_id = 0 } } };

    auto status = info->create(&mk, linecardId, attr_count, attr_list);

    if (status == OTAI_STATUS_SUCCESS)
    {
        *objectId = mk.objectkey.key.object_id;
    }

    return status;
}

otai_status_t VendorOtai::remove(
    _In_ otai_object_type_t objectType,
    _In_ otai_object_id_t objectId)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    auto info = otai_metadata_get_object_type_info(objectType);

    if (!info)
    {
        SWSS_LOG_ERROR("unable to get info for object type: %s",
            otai_serialize_object_type(objectType).c_str());

        return OTAI_STATUS_FAILURE;
    }

    if (!info->remove)
    {
        SWSS_LOG_ERROR("object type %s has no remove method",
            otai_serialize_object_type(objectType).c_str());

        return OTAI_STATUS_FAILURE;
    }

    if (info->isnonobjectid)
    {
        SWSS_LOG_ERROR("passed non object id as object id!: %s",
            otai_serialize_object_type(objectType).c_str());

        return OTAI_STATUS_FAILURE;
    }

    otai_object_meta_key_t mk = { .objecttype = objectType, .objectkey = {.key = {.object_id = objectId } } };

    return info->remove(&mk);
}

otai_status_t VendorOtai::set(
    _In_ otai_object_type_t objectType,
    _In_ otai_object_id_t objectId,
    _In_ const otai_attribute_t* attr)
{
    std::unique_lock<std::mutex> _lock(m_apimutex);
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    auto info = otai_metadata_get_object_type_info(objectType);

    if (!info)
    {
        SWSS_LOG_ERROR("unable to get info for object type: %s",
            otai_serialize_object_type(objectType).c_str());

        return OTAI_STATUS_FAILURE;
    }

    if (!info->set)
    {
        SWSS_LOG_ERROR("object type %s has no set method",
            otai_serialize_object_type(objectType).c_str());

        return OTAI_STATUS_FAILURE;
    }

    if (info->isnonobjectid)
    {
        SWSS_LOG_ERROR("passed non object id as object id!: %s",
            otai_serialize_object_type(objectType).c_str());

        return OTAI_STATUS_FAILURE;
    }

    otai_object_meta_key_t mk = { .objecttype = objectType, .objectkey = {.key = {.object_id = objectId } } };

    return info->set(&mk, attr);
}

otai_status_t VendorOtai::get(
    _In_ otai_object_type_t objectType,
    _In_ otai_object_id_t objectId,
    _In_ uint32_t attr_count,
    _Inout_ otai_attribute_t* attr_list)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    auto info = otai_metadata_get_object_type_info(objectType);

    if (!info)
    {
        SWSS_LOG_ERROR("unable to get info for object type: %s",
            otai_serialize_object_type(objectType).c_str());

        return OTAI_STATUS_FAILURE;
    }

    if (!info->get)
    {
        SWSS_LOG_ERROR("object type %s has no get method",
            otai_serialize_object_type(objectType).c_str());

        return OTAI_STATUS_FAILURE;
    }

    if (info->isnonobjectid)
    {
        SWSS_LOG_ERROR("passed non object id as object id!: %s",
            otai_serialize_object_type(objectType).c_str());

        return OTAI_STATUS_FAILURE;
    }

    otai_object_meta_key_t mk = { .objecttype = objectType, .objectkey = {.key = {.object_id = objectId } } };

    return info->get(&mk, attr_count, attr_list);
}


// STATS

otai_status_t VendorOtai::getStats(
    _In_ otai_object_type_t object_type,
    _In_ otai_object_id_t object_id,
    _In_ uint32_t number_of_counters,
    _In_ const otai_stat_id_t* counter_ids,
    _Out_ otai_stat_value_t* counters)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    otai_status_t(*ptr)(
        _In_ otai_object_id_t id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t * counter_ids,
        _Out_ otai_stat_value_t * counters) = nullptr;

    if (!counter_ids || !counters)
    {
        SWSS_LOG_ERROR("NULL pointer function argument");
        return OTAI_STATUS_INVALID_PARAMETER;
    }

    switch ((int)object_type)
    {
    case OTAI_OBJECT_TYPE_LINECARD:
        ptr = m_apis.linecard_api->get_linecard_stats;
        break;
    case OTAI_OBJECT_TYPE_PORT:
        ptr = m_apis.port_api->get_port_stats;
        break;
    case OTAI_OBJECT_TYPE_TRANSCEIVER:
        ptr = m_apis.transceiver_api->get_transceiver_stats;
        break;
    case OTAI_OBJECT_TYPE_LOGICALCHANNEL:
        ptr = m_apis.logicalchannel_api->get_logicalchannel_stats;
        break;
    case OTAI_OBJECT_TYPE_OTN:
        ptr = m_apis.otn_api->get_otn_stats;
        break;
    case OTAI_OBJECT_TYPE_ETHERNET:
        ptr = m_apis.ethernet_api->get_ethernet_stats;
        break;
    case OTAI_OBJECT_TYPE_PHYSICALCHANNEL:
        ptr = m_apis.physicalchannel_api->get_physicalchannel_stats;
        break;
    case OTAI_OBJECT_TYPE_OCH:
        ptr = m_apis.och_api->get_och_stats;
        break;
    case OTAI_OBJECT_TYPE_LLDP:
        ptr = m_apis.lldp_api->get_lldp_stats;
        break;
    case OTAI_OBJECT_TYPE_ASSIGNMENT:
        ptr = m_apis.assignment_api->get_assignment_stats;
        break;
    case OTAI_OBJECT_TYPE_INTERFACE:
        ptr = m_apis.interface_api->get_interface_stats;
        break;
    case OTAI_OBJECT_TYPE_OA:
        ptr = m_apis.oa_api->get_oa_stats;
        break;
    case OTAI_OBJECT_TYPE_OSC:
        ptr = m_apis.osc_api->get_osc_stats;
        break;
    case OTAI_OBJECT_TYPE_APS:
        ptr = m_apis.aps_api->get_aps_stats;
        break;
    case OTAI_OBJECT_TYPE_APSPORT:
        ptr = m_apis.apsport_api->get_apsport_stats;
        break;
    case OTAI_OBJECT_TYPE_ATTENUATOR:
        ptr = m_apis.attenuator_api->get_attenuator_stats;
        break;
    case OTAI_OBJECT_TYPE_OCM:
        ptr = m_apis.ocm_api->get_ocm_stats;
        break;
    case OTAI_OBJECT_TYPE_OTDR:
        ptr = m_apis.otdr_api->get_otdr_stats;
        break;
    default:
        SWSS_LOG_ERROR("not implemented, FIXME");
        return OTAI_STATUS_FAILURE;
    }

    if (nullptr == ptr)
    {
        SWSS_LOG_ERROR("not implemented, FIXME");
        return OTAI_STATUS_FAILURE;
    }

    return ptr(object_id, number_of_counters, counter_ids, counters);
}

otai_status_t VendorOtai::getStatsExt(
    _In_ otai_object_type_t object_type,
    _In_ otai_object_id_t object_id,
    _In_ uint32_t number_of_counters,
    _In_ const otai_stat_id_t* counter_ids,
    _In_ otai_stats_mode_t mode,
    _Out_ otai_stat_value_t* counters)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    otai_status_t(*ptr)(
        _In_ otai_object_id_t id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t * counter_ids,
        _In_ otai_stats_mode_t mode,
        _Out_ otai_stat_value_t * counters) = nullptr;

    switch ((int)object_type)
    {
    default:
        SWSS_LOG_ERROR("not implemented, FIXME");
        return OTAI_STATUS_FAILURE;
    }
    return ptr(object_id, number_of_counters, counter_ids, mode, counters);
}

otai_status_t VendorOtai::clearStats(
    _In_ otai_object_type_t object_type,
    _In_ otai_object_id_t object_id,
    _In_ uint32_t number_of_counters,
    _In_ const otai_stat_id_t* counter_ids)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();

    otai_status_t(*ptr)(
        _In_ otai_object_id_t id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t * counter_ids) = nullptr;

    switch ((int)object_type)
    {
    case OTAI_OBJECT_TYPE_LINECARD:
        ptr = m_apis.linecard_api->clear_linecard_stats;
        break;
    case OTAI_OBJECT_TYPE_PORT:
        ptr = m_apis.port_api->clear_port_stats;
        break;
    case OTAI_OBJECT_TYPE_TRANSCEIVER:
        ptr = m_apis.transceiver_api->clear_transceiver_stats;
        break;
    case OTAI_OBJECT_TYPE_LOGICALCHANNEL:
        ptr = m_apis.logicalchannel_api->clear_logicalchannel_stats;
        break;
    case OTAI_OBJECT_TYPE_OTN:
        ptr = m_apis.otn_api->clear_otn_stats;
        break;
    case OTAI_OBJECT_TYPE_ETHERNET:
        ptr = m_apis.ethernet_api->clear_ethernet_stats;
        break;
    case OTAI_OBJECT_TYPE_PHYSICALCHANNEL:
        ptr = m_apis.physicalchannel_api->clear_physicalchannel_stats;
        break;
    case OTAI_OBJECT_TYPE_OCH:
        ptr = m_apis.och_api->clear_och_stats;
        break;
    case OTAI_OBJECT_TYPE_LLDP:
        ptr = m_apis.lldp_api->clear_lldp_stats;
        break;
    case OTAI_OBJECT_TYPE_ASSIGNMENT:
        ptr = m_apis.assignment_api->clear_assignment_stats;
        break;
    case OTAI_OBJECT_TYPE_INTERFACE:
        ptr = m_apis.interface_api->clear_interface_stats;
        break;
    case OTAI_OBJECT_TYPE_OA:
        ptr = m_apis.oa_api->clear_oa_stats;
        break;
    case OTAI_OBJECT_TYPE_OSC:
        ptr = m_apis.osc_api->clear_osc_stats;
        break;
    case OTAI_OBJECT_TYPE_APS:
        ptr = m_apis.aps_api->clear_aps_stats;
        break;
    case OTAI_OBJECT_TYPE_APSPORT:
        ptr = m_apis.apsport_api->clear_apsport_stats;
        break;
    case OTAI_OBJECT_TYPE_ATTENUATOR:
        ptr = m_apis.attenuator_api->clear_attenuator_stats;
        break;
    default:
        SWSS_LOG_ERROR("not implemented, FIXME");
        return OTAI_STATUS_FAILURE;
    }

    if (nullptr == ptr)
    {
        SWSS_LOG_ERROR("not implemented, FIXME");
        return OTAI_STATUS_FAILURE;
    }

    return ptr(object_id, number_of_counters, counter_ids);
}

// OTAI API
otai_object_type_t VendorOtai::objectTypeQuery(
    _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (!m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: OTAI API not initialized", __PRETTY_FUNCTION__);

        return OTAI_OBJECT_TYPE_NULL;
    }

    return otai_object_type_query(objectId);
}

otai_object_id_t VendorOtai::linecardIdQuery(
    _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (!m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: OTAI API not initialized", __PRETTY_FUNCTION__);

        return OTAI_NULL_OBJECT_ID;
    }

    return otai_linecard_id_query(objectId);
}

otai_status_t VendorOtai::logSet(
    _In_ otai_api_t api,
    _In_ otai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    return otai_log_set(api, log_level);
}
