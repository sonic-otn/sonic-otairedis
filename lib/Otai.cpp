#include "Otai.h"
#include "OtaiInternal.h"
#include "ContextConfigContainer.h"

#include "meta/Meta.h"
#include "meta/otai_serialize.h"


using namespace otairedis;
using namespace std::placeholders;

#define REDIS_CHECK_API_INITIALIZED()                                       \
    if (!m_apiInitialized) {                                                \
        SWSS_LOG_ERROR("%s: api not initialized", __PRETTY_FUNCTION__);     \
        return OTAI_STATUS_FAILURE; }

#define REDIS_CHECK_CONTEXT(oid)                                            \
    auto _globalContext = VirtualObjectIdManager::getGlobalContext(oid);    \
    auto context = getContext(_globalContext);                              \
    if (context == nullptr) {                                               \
        SWSS_LOG_ERROR("no context at index %u for oid %s",                 \
                _globalContext,                                             \
                otai_serialize_object_id(oid).c_str());                      \
        return OTAI_STATUS_FAILURE; }

Otai::Otai()
{
    SWSS_LOG_ENTER();

    m_apiInitialized = false;
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

    const char* contextConfig = service_method_table->profile_get_value(0, OTAI_REDIS_KEY_CONTEXT_CONFIG);

    auto ccc = ContextConfigContainer::loadFromFile(contextConfig);

    for (auto&cc: ccc->getAllContextConfigs())
    {
        auto context = std::make_shared<Context>(cc, std::bind(&Otai::handle_notification, this, _1, _2));

        m_contextMap[cc->m_guid] = context;
    }

    m_apiInitialized = true;

    return OTAI_STATUS_SUCCESS;
}

otai_status_t Otai::uninitialize(void)
{
    SWSS_LOG_ENTER();
    REDIS_CHECK_API_INITIALIZED();

    SWSS_LOG_NOTICE("begin");

    m_contextMap.clear();

    m_apiInitialized = false;

    SWSS_LOG_NOTICE("end");

    return OTAI_STATUS_SUCCESS;
}

otai_status_t Otai::linkCheck(_Out_ bool *up)
{
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
    REDIS_CHECK_API_INITIALIZED();

    REDIS_CHECK_CONTEXT(linecardId);

    if (objectType == OTAI_OBJECT_TYPE_LINECARD && attr_count > 0 && attr_list)
    {
        uint32_t globalContext = 0; // default

        SWSS_LOG_NOTICE("request linecard create with context %u", globalContext);

        context = getContext(globalContext);

        if (context == nullptr)
        {
            SWSS_LOG_ERROR("no global context defined at index %u", globalContext);

            return OTAI_STATUS_FAILURE;
        }
    }

    auto status = context->m_meta->create(
            objectType,
            objectId,
            linecardId,
            attr_count,
            attr_list);

    return status;
}

otai_status_t Otai::remove(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId)
{
    MUTEX();
    SWSS_LOG_ENTER();
    REDIS_CHECK_API_INITIALIZED();
    REDIS_CHECK_CONTEXT(objectId);

    return context->m_meta->remove(objectType, objectId);
}

otai_status_t Otai::set(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ const otai_attribute_t *attr)
{
    MUTEX();
    SWSS_LOG_ENTER();
    REDIS_CHECK_API_INITIALIZED();

    if (RedisRemoteOtaiInterface::isRedisAttribute(objectType, attr))
    {
        // skip metadata if attribute is redis extension attribute

        // TODO this is setting on all contexts, but maybe we want one specific?
        // and do set on all if objectId == NULL

        bool success = true;

        for (auto& kvp: m_contextMap)
        {
            otai_status_t status = kvp.second->m_redisOtai->set(objectType, objectId, attr);

            success &= (status == OTAI_STATUS_SUCCESS);

            SWSS_LOG_INFO("setting attribute 0x%x status: %s",
                    attr->id,
                    otai_serialize_status(status).c_str());
        }

        return success ? OTAI_STATUS_SUCCESS : OTAI_STATUS_FAILURE;
    }

    REDIS_CHECK_CONTEXT(objectId);

    return context->m_meta->set(objectType, objectId, attr);
}

otai_status_t Otai::get(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ uint32_t attr_count,
        _Inout_ otai_attribute_t *attr_list)
{
    MUTEX();
    SWSS_LOG_ENTER();
    REDIS_CHECK_API_INITIALIZED();
    REDIS_CHECK_CONTEXT(objectId);

    return context->m_meta->get(
            objectType,
            objectId,
            attr_count,
            attr_list);
}

// OTAI API

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
    REDIS_CHECK_API_INITIALIZED();
    REDIS_CHECK_CONTEXT(object_id);

    return context->m_meta->getStats(
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
    REDIS_CHECK_API_INITIALIZED();
    REDIS_CHECK_CONTEXT(object_id);

    return context->m_meta->getStatsExt(
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
    REDIS_CHECK_API_INITIALIZED();
    REDIS_CHECK_CONTEXT(object_id);

    return context->m_meta->clearStats(
            object_type,
            object_id,
            number_of_counters,
            counter_ids);
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

    return VirtualObjectIdManager::objectTypeQuery(objectId);
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

    return VirtualObjectIdManager::linecardIdQuery(objectId);
}

otai_status_t Otai::logSet(
        _In_ otai_api_t api,
        _In_ otai_log_level_t log_level)
{
    MUTEX();
    SWSS_LOG_ENTER();
    REDIS_CHECK_API_INITIALIZED();

    for (auto&kvp: m_contextMap)
    {
        kvp.second->m_meta->logSet(api, log_level);
    }

    return OTAI_STATUS_SUCCESS;
}

/*
 * NOTE: Notifications during linecard create and linecard remove.
 *
 * It is possible that when we create linecard we will immediately start getting
 * notifications from it, and it may happen that this linecard will not be yet
 * put to linecard container and notification won't find it. But before
 * notification will be processed it will first try to acquire mutex, so create
 * linecard function will end and linecard will be put inside container.
 *
 * Similar it can happen that we receive notification when we are removing
 * linecard, then linecard will be removed from linecard container and notification
 * will not find existing linecard, but that's ok since linecard was removed, and
 * notification can be ignored.
 */

otai_linecard_notifications_t Otai::handle_notification(
        _In_ std::shared_ptr<Notification> notification,
        _In_ Context* context)
{
    MUTEX();
    SWSS_LOG_ENTER();

    if (!m_apiInitialized)
    {
        SWSS_LOG_ERROR("%s: api not initialized", __PRETTY_FUNCTION__);

        return {nullptr, nullptr, nullptr, nullptr};
    }

    return context->m_redisOtai->syncProcessNotification(notification);
}

std::shared_ptr<Context> Otai::getContext(
        _In_ uint32_t globalContext)
{
    SWSS_LOG_ENTER();

    auto it = m_contextMap.find(globalContext);

    if (it == m_contextMap.end())
        return nullptr;

    return it->second;
}

std::string joinFieldValues(
        _In_ const std::vector<swss::FieldValueTuple> &values)
{
    SWSS_LOG_ENTER();

    std::stringstream ss;

    for (size_t i = 0; i < values.size(); ++i)
    {
        const std::string &str_attr_id = fvField(values[i]);
        const std::string &str_attr_value = fvValue(values[i]);

        if (i != 0)
        {
            ss << "|";
        }

        ss << str_attr_id << "=" << str_attr_value;
    }

    return ss.str();
}

std::vector<swss::FieldValueTuple> serialize_counter_id_list(
        _In_ const otai_enum_metadata_t *stats_enum,
        _In_ uint32_t count,
        _In_ const otai_stat_id_t*counter_id_list)
{
    SWSS_LOG_ENTER();

    std::vector<swss::FieldValueTuple> values;

    for (uint32_t i = 0; i < count; i++)
    {
        const char *name = otai_metadata_get_enum_value_name(stats_enum, counter_id_list[i]);

        if (name == NULL)
        {
            SWSS_LOG_THROW("failed to find enum %d in %s", counter_id_list[i], stats_enum->name);
        }

        values.emplace_back(name, "");
    }

    return values;
}

std::vector<swss::FieldValueTuple> serialize_alarm_id_list(
        _In_ const otai_enum_metadata_t *alarms_enum,
        _In_ uint32_t number_of_alarms,
        _In_ const otai_alarm_type_t *alarm_id_list)
{
    SWSS_LOG_ENTER();

    std::vector<swss::FieldValueTuple> values;

    for (uint32_t i = 0; i < number_of_alarms; i++)
    {
        const char *name = otai_metadata_get_enum_value_name(alarms_enum, alarm_id_list[i]);

        if (name == NULL)
        {
            SWSS_LOG_THROW("failed to find enum %d in %s", alarm_id_list[i], alarms_enum->name);
        }

        values.emplace_back(name, "");
    }

    return values;
}

