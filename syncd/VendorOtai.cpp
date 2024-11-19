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

#define VENDOR_CHECK_META_OBJECT_TYPE()                                      \
    auto info = otai_metadata_get_object_type_info(object_type);            \
    if (!info) {                                                            \
        SWSS_LOG_ERROR("unable to get info for object type: %s",            \
            otai_serialize_object_type(object_type).c_str());               \
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
    _In_ otai_object_type_t object_type,
    _Out_ otai_object_id_t* objectId,
    _In_ otai_object_id_t linecardId,
    _In_ uint32_t attr_count,
    _In_ const otai_attribute_t* attr_list)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();
    VENDOR_CHECK_META_OBJECT_TYPE();

    if (!info->create)
    {
        SWSS_LOG_ERROR("object type %s has no create method",
            otai_serialize_object_type(object_type).c_str());

        return OTAI_STATUS_FAILURE;
    }

    if (info->isnonobjectid)
    {
        SWSS_LOG_ERROR("passed non object id as object id!: %s",
            otai_serialize_object_type(object_type).c_str());

        return OTAI_STATUS_FAILURE;
    }

    otai_object_meta_key_t mk = { .objecttype = object_type, .objectkey = {.key = {.object_id = 0 } } };

    auto status = info->create(&mk, linecardId, attr_count, attr_list);

    if (status == OTAI_STATUS_SUCCESS)
    {
        *objectId = mk.objectkey.key.object_id;
    }

    return status;
}

otai_status_t VendorOtai::remove(
    _In_ otai_object_type_t object_type,
    _In_ otai_object_id_t objectId)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();
    VENDOR_CHECK_META_OBJECT_TYPE();

    if (!info->remove)
    {
        SWSS_LOG_ERROR("object type %s has no remove method",
            otai_serialize_object_type(object_type).c_str());

        return OTAI_STATUS_FAILURE;
    }

    if (info->isnonobjectid)
    {
        SWSS_LOG_ERROR("passed non object id as object id!: %s",
            otai_serialize_object_type(object_type).c_str());

        return OTAI_STATUS_FAILURE;
    }

    otai_object_meta_key_t mk = { .objecttype = object_type, .objectkey = {.key = {.object_id = objectId } } };

    return info->remove(&mk);
}

otai_status_t VendorOtai::set(
    _In_ otai_object_type_t object_type,
    _In_ otai_object_id_t objectId,
    _In_ const otai_attribute_t* attr)
{
    std::unique_lock<std::mutex> _lock(m_apimutex);
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();
    VENDOR_CHECK_META_OBJECT_TYPE();

    if (!info->set)
    {
        SWSS_LOG_ERROR("object type %s has no set method",
            otai_serialize_object_type(object_type).c_str());

        return OTAI_STATUS_FAILURE;
    }

    if (info->isnonobjectid)
    {
        SWSS_LOG_ERROR("passed non object id as object id!: %s",
            otai_serialize_object_type(object_type).c_str());

        return OTAI_STATUS_FAILURE;
    }

    otai_object_meta_key_t mk = { .objecttype = object_type, .objectkey = {.key = {.object_id = objectId } } };

    return info->set(&mk, attr);
}

otai_status_t VendorOtai::get(
    _In_ otai_object_type_t object_type,
    _In_ otai_object_id_t objectId,
    _In_ uint32_t attr_count,
    _Inout_ otai_attribute_t* attr_list)
{
    MUTEX();
    SWSS_LOG_ENTER();
    VENDOR_CHECK_API_INITIALIZED();
    VENDOR_CHECK_META_OBJECT_TYPE();

    if (!info->get)
    {
        SWSS_LOG_ERROR("object type %s has no get method",
            otai_serialize_object_type(object_type).c_str());

        return OTAI_STATUS_FAILURE;
    }

    if (info->isnonobjectid)
    {
        SWSS_LOG_ERROR("passed non object id as object id!: %s",
            otai_serialize_object_type(object_type).c_str());

        return OTAI_STATUS_FAILURE;
    }

    otai_object_meta_key_t mk = { .objecttype = object_type, .objectkey = {.key = {.object_id = objectId } } };

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
    VENDOR_CHECK_META_OBJECT_TYPE();

    if (!counter_ids || !counters)
    {
        SWSS_LOG_ERROR("NULL pointer function argument");
        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (!info->getstats)
    {
        SWSS_LOG_ERROR("object type %s has no getstats method",
            otai_serialize_object_type(object_type).c_str());

        return OTAI_STATUS_FAILURE;
    }

    otai_object_meta_key_t mk = { .objecttype = object_type, .objectkey = { .key = { .object_id = object_id} } };
    return info->getstats(&mk, number_of_counters, counter_ids, counters);
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
    VENDOR_CHECK_META_OBJECT_TYPE();

    if (!counter_ids || !counters)
    {
        SWSS_LOG_ERROR("NULL pointer function argument");
        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (!info->getstatsext)
    {
        SWSS_LOG_ERROR("object type %s has no getstatsext method",
            otai_serialize_object_type(object_type).c_str());

        return OTAI_STATUS_FAILURE;
    }

    otai_object_meta_key_t mk = { .objecttype = object_type, .objectkey = { .key = { .object_id = object_id} } };
    return info->getstatsext(&mk, number_of_counters, counter_ids, mode, counters);
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
    VENDOR_CHECK_META_OBJECT_TYPE();

    if (!info->clearstats)
    {
        SWSS_LOG_ERROR("object type %s has no clearstats method",
            otai_serialize_object_type(object_type).c_str());

        return OTAI_STATUS_FAILURE;
    }

    otai_object_meta_key_t mk = { .objecttype = object_type, .objectkey = { .key = { .object_id = object_id} } };
    return info->clearstats(&mk, number_of_counters, counter_ids);
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
