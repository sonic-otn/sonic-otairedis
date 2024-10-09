#include "RedisRemoteOtaiInterface.h"
#include "Utils.h"
#include "Recorder.h"
#include "VirtualObjectIdManager.h"
#include "SkipRecordAttrContainer.h"
#include "LinecardContainer.h"

#include "otairediscommon.h"

#include "meta/NotificationFactory.h"
#include "meta/otai_serialize.h"
#include "meta/OtaiAttributeList.h"

#include <inttypes.h>

using namespace otairedis;
using namespace otaimeta;
using namespace std::placeholders;

std::string joinFieldValues(
        _In_ const std::vector<swss::FieldValueTuple> &values);

std::vector<swss::FieldValueTuple> serialize_counter_id_list(
        _In_ const otai_enum_metadata_t *stats_enum,
        _In_ uint32_t count,
        _In_ const otai_stat_id_t *counter_id_list);

std::vector<swss::FieldValueTuple> serialize_alarm_id_list(
        _In_ const otai_enum_metadata_t *alarms_enum,
        _In_ uint32_t number_of_alarms,
        _In_ const otai_alarm_type_t *alarm_id_list);

RedisRemoteOtaiInterface::RedisRemoteOtaiInterface(
        _In_ std::shared_ptr<ContextConfig> contextConfig,
        _In_ std::function<otai_linecard_notifications_t(std::shared_ptr<Notification>)> notificationCallback,
        _In_ std::shared_ptr<Recorder> recorder):
    m_contextConfig(contextConfig),
    m_redisCommunicationMode(OTAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC),
    m_recorder(recorder),
    m_notificationCallback(notificationCallback)
{
    SWSS_LOG_ENTER();

    m_initialized = false;

    initialize(0, nullptr);
}

RedisRemoteOtaiInterface::~RedisRemoteOtaiInterface()
{
    SWSS_LOG_ENTER();

    if (m_initialized)
    {
        uninitialize();
    }
}

otai_status_t RedisRemoteOtaiInterface::initialize(
        _In_ uint64_t flags,
        _In_ const otai_service_method_table_t *service_method_table)
{
    SWSS_LOG_ENTER();

    if (m_initialized)
    {
        SWSS_LOG_ERROR("already initialized");

        return OTAI_STATUS_FAILURE;
    }

    m_skipRecordAttrContainer = std::make_shared<SkipRecordAttrContainer>();

    m_asicInitViewMode = false; // default mode is apply mode
    m_useTempView = false;
    m_syncMode = false;
    m_redisCommunicationMode = OTAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC;

    m_communicationChannel = std::make_shared<RedisChannel>(
            m_contextConfig->m_dbAsic,
            std::bind(&RedisRemoteOtaiInterface::handleNotification, this, _1, _2, _3));

    m_responseTimeoutMs = m_communicationChannel->getResponseTimeout();

    m_db = std::make_shared<swss::DBConnector>(m_contextConfig->m_dbAsic, 0);

    m_redisVidIndexGenerator = std::make_shared<RedisVidIndexGenerator>(m_db, REDIS_KEY_VIDCOUNTER);

    clear_local_state();

    // TODO what will happen when we receive notification in init view mode ?

    m_initialized = true;

    return OTAI_STATUS_SUCCESS;
}

otai_status_t RedisRemoteOtaiInterface::uninitialize(void)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("begin");

    if (!m_initialized)
    {
        SWSS_LOG_ERROR("not initialized");

        return OTAI_STATUS_FAILURE;
    }

    m_communicationChannel = nullptr; // will stop thread

    // clear local state after stopping threads

    clear_local_state();

    m_initialized = false;

    SWSS_LOG_NOTICE("end");

    return OTAI_STATUS_SUCCESS;
}

otai_status_t RedisRemoteOtaiInterface::linkCheck(_Out_ bool *up)
{
    return OTAI_STATUS_SUCCESS;
}

std::string RedisRemoteOtaiInterface::getHardwareInfo(
        _In_ uint32_t attrCount,
        _In_ const otai_attribute_t *attrList) const
{
    SWSS_LOG_ENTER();

    return "";
}


otai_status_t RedisRemoteOtaiInterface::create(
        _In_ otai_object_type_t objectType,
        _Out_ otai_object_id_t* objectId,
        _In_ otai_object_id_t linecardId,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    *objectId = OTAI_NULL_OBJECT_ID;

    if (objectType == OTAI_OBJECT_TYPE_LINECARD)
    {
        // for given hardware info we always return same linecard id,
        // this is required since we could be performing warm boot here

        auto hwinfo = getHardwareInfo(attr_count, attr_list);

        linecardId = m_virtualObjectIdManager->allocateNewLinecardObjectId(hwinfo);

        *objectId = linecardId;

        if (linecardId == OTAI_NULL_OBJECT_ID)
        {
            SWSS_LOG_ERROR("linecard ID allocation failed");

            return OTAI_STATUS_FAILURE;
        }
    }
    else
    {
        *objectId = m_virtualObjectIdManager->allocateNewObjectId(objectType, linecardId);
    }

    if (*objectId == OTAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_ERROR("failed to create %s, with linecard id: %s",
                otai_serialize_object_type(objectType).c_str(),
                otai_serialize_object_id(linecardId).c_str());

        return OTAI_STATUS_INSUFFICIENT_RESOURCES;
    }

    // NOTE: objectId was allocated by the caller

    auto status = create(
            objectType,
            otai_serialize_object_id(*objectId),
            attr_count,
            attr_list);

    if (objectType == OTAI_OBJECT_TYPE_LINECARD && status == OTAI_STATUS_SUCCESS)
    {
        /*
         * When doing CREATE operation user may want to update notification
         * pointers, since notifications can be defined per linecard we need to
         * update them.
         *
         * TODO: should be moved inside to redis_generic_create
         */

        auto sw = std::make_shared<Linecard>(*objectId, attr_count, attr_list);

        m_linecardContainer->insert(sw);
    }
    else if (status != OTAI_STATUS_SUCCESS)
    {
        // if create failed, then release allocated object
        m_virtualObjectIdManager->releaseObjectId(*objectId);
    }

    return status;
}

otai_status_t RedisRemoteOtaiInterface::remove(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    auto status = remove(
            objectType,
            otai_serialize_object_id(objectId));

    if (objectType == OTAI_OBJECT_TYPE_LINECARD && status == OTAI_STATUS_SUCCESS)
    {
        SWSS_LOG_NOTICE("removing linecard id %s", otai_serialize_object_id(objectId).c_str());

        m_virtualObjectIdManager->releaseObjectId(objectId);

        // remove linecard from container
        m_linecardContainer->removeLinecard(objectId);
    }

    return status;
}

otai_status_t RedisRemoteOtaiInterface::setRedisExtensionAttribute(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    if (attr == nullptr)
    {
        SWSS_LOG_ERROR("attr pointer is null");

        return OTAI_STATUS_FAILURE;
    }

    /*
     * NOTE: that this will work without
     * linecard being created.
     */

    switch (attr->id)
    {
        case OTAI_REDIS_LINECARD_ATTR_PERFORM_LOG_ROTATE:

            if (m_recorder)
            {
                m_recorder->requestLogRotate();
            }

            return OTAI_STATUS_SUCCESS;

        case OTAI_REDIS_LINECARD_ATTR_RECORD:

            if (m_recorder)
            {
                m_recorder->enableRecording(attr->value.booldata);
            }

            return OTAI_STATUS_SUCCESS;

        case OTAI_REDIS_LINECARD_ATTR_NOTIFY_SYNCD:

            return otai_redis_notify_syncd(objectId, attr);

        case OTAI_REDIS_LINECARD_ATTR_USE_TEMP_VIEW:

            m_useTempView = attr->value.booldata;

            return OTAI_STATUS_SUCCESS;

        case OTAI_REDIS_LINECARD_ATTR_RECORD_STATS:

            m_recorder->recordStats(attr->value.booldata);

            return OTAI_STATUS_SUCCESS;

        case OTAI_REDIS_LINECARD_ATTR_SYNC_OPERATION_RESPONSE_TIMEOUT:

            m_responseTimeoutMs = attr->value.u64;

            m_communicationChannel->setResponseTimeout(m_responseTimeoutMs);

            SWSS_LOG_NOTICE("set response timeout to %" PRIu64 " ms", m_responseTimeoutMs);

            return OTAI_STATUS_SUCCESS;

        case OTAI_REDIS_LINECARD_ATTR_SYNC_MODE:

            SWSS_LOG_WARN("sync mode is depreacated, use communication mode");

            m_syncMode = attr->value.booldata;

            if (m_syncMode)
            {
                SWSS_LOG_NOTICE("disabling buffered pipeline in sync mode");

                m_communicationChannel->setBuffered(false);
            }

            return OTAI_STATUS_SUCCESS;

        case OTAI_REDIS_LINECARD_ATTR_REDIS_COMMUNICATION_MODE:

            m_redisCommunicationMode = (otai_redis_communication_mode_t)attr->value.s32;

            m_communicationChannel = nullptr;

            switch (m_redisCommunicationMode)
            {
                case OTAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC:

                    SWSS_LOG_NOTICE("enabling redis async mode");

                    m_syncMode = false;

                    m_communicationChannel = std::make_shared<RedisChannel>(
                            m_contextConfig->m_dbAsic,
                            std::bind(&RedisRemoteOtaiInterface::handleNotification, this, _1, _2, _3));

                    m_communicationChannel->setResponseTimeout(m_responseTimeoutMs);

                    m_communicationChannel->setBuffered(true);

                    return OTAI_STATUS_SUCCESS;

                case OTAI_REDIS_COMMUNICATION_MODE_REDIS_SYNC:

                    SWSS_LOG_NOTICE("enabling redis sync mode");

                    m_syncMode = true;

                    m_communicationChannel = std::make_shared<RedisChannel>(
                            m_contextConfig->m_dbAsic,
                            std::bind(&RedisRemoteOtaiInterface::handleNotification, this, _1, _2, _3));

                    m_communicationChannel->setResponseTimeout(m_responseTimeoutMs);

                    m_communicationChannel->setBuffered(false);

                    return OTAI_STATUS_SUCCESS;

                default:

                    SWSS_LOG_ERROR("invalid communication mode value: %d", m_redisCommunicationMode);

                    return OTAI_STATUS_NOT_SUPPORTED;
            }

        case OTAI_REDIS_LINECARD_ATTR_USE_PIPELINE:

            if (m_syncMode)
            {
                SWSS_LOG_WARN("use pipeline is not supported in sync mode");

                return OTAI_STATUS_NOT_SUPPORTED;
            }

            m_communicationChannel->setBuffered(attr->value.booldata);

            return OTAI_STATUS_SUCCESS;

        case OTAI_REDIS_LINECARD_ATTR_FLUSH:

            m_communicationChannel->flush();

            return OTAI_STATUS_SUCCESS;

        case OTAI_REDIS_LINECARD_ATTR_RECORDING_OUTPUT_DIR:

            if (m_recorder)
            {
                m_recorder->setRecordingOutputDirectory(*attr);
            }

            return OTAI_STATUS_SUCCESS;

        case OTAI_REDIS_LINECARD_ATTR_RECORDING_FILENAME:

            if (m_recorder)
            {
                m_recorder->setRecordingFilename(*attr);
            }

            return OTAI_STATUS_SUCCESS;
            
        default:
            break;
    }

    SWSS_LOG_ERROR("unknown redis extension attribute: %d", attr->id);

    return OTAI_STATUS_FAILURE;
}

otai_status_t RedisRemoteOtaiInterface::set(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    if (RedisRemoteOtaiInterface::isRedisAttribute(objectType, attr))
    {
        return setRedisExtensionAttribute(objectType, objectId, attr);
    }

    auto status = set(
            objectType,
            otai_serialize_object_id(objectId),
            attr);

    if (objectType == OTAI_OBJECT_TYPE_LINECARD && status == OTAI_STATUS_SUCCESS)
    {
        auto sw = m_linecardContainer->getLinecard(objectId);

        if (!sw)
        {
            SWSS_LOG_THROW("failed to find linecard %s in container",
                    otai_serialize_object_id(objectId).c_str());
        }

        /*
         * When doing SET operation user may want to update notification
         * pointers.
         */

        sw->updateNotifications(1, attr);
    }

    return status;
}

otai_status_t RedisRemoteOtaiInterface::get(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ uint32_t attr_count,
        _Inout_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return get(
            objectType,
            otai_serialize_object_id(objectId),
            attr_count,
            attr_list);
}

otai_status_t RedisRemoteOtaiInterface::create(
        _In_ otai_object_type_t object_type,
        _In_ const std::string& serializedObjectId,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    auto entry = OtaiAttributeList::serialize_attr_list(
            object_type,
            attr_count,
            attr_list,
            false);

    if (entry.empty())
    {
        // make sure that we put object into db
        // even if there are no attributes set
        swss::FieldValueTuple null("NULL", "NULL");

        entry.push_back(null);
    }

    auto serializedObjectType = otai_serialize_object_type(object_type);

    const std::string key = serializedObjectType + ":" + serializedObjectId;

    SWSS_LOG_NOTICE("generic create key: %s, fields: %zu", key.c_str(), entry.size());

    m_recorder->recordGenericCreate(key, entry);

    m_communicationChannel->set(key, entry, REDIS_ASIC_STATE_COMMAND_CREATE);

    auto status = waitForResponse(OTAI_COMMON_API_CREATE);
    SWSS_LOG_NOTICE("generic create key end: %s, fields: %zu", key.c_str(), entry.size());

    m_recorder->recordGenericCreateResponse(status);

    return status;
}

otai_status_t RedisRemoteOtaiInterface::remove(
        _In_ otai_object_type_t objectType,
        _In_ const std::string& serializedObjectId)
{
    SWSS_LOG_ENTER();

    auto serializedObjectType = otai_serialize_object_type(objectType);

    const std::string key = serializedObjectType + ":" + serializedObjectId;

    SWSS_LOG_NOTICE("generic remove key: %s", key.c_str());

    m_recorder->recordGenericRemove(key);

    m_communicationChannel->del(key, REDIS_ASIC_STATE_COMMAND_REMOVE);

    auto status = waitForResponse(OTAI_COMMON_API_REMOVE);

    m_recorder->recordGenericRemoveResponse(status);

    return status;
}

otai_status_t RedisRemoteOtaiInterface::set(
        _In_ otai_object_type_t objectType,
        _In_ const std::string &serializedObjectId,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    auto entry = OtaiAttributeList::serialize_attr_list(
            objectType,
            1,
            attr,
            false);

    auto serializedObjectType = otai_serialize_object_type(objectType);

    std::string key = serializedObjectType + ":" + serializedObjectId;

    SWSS_LOG_DEBUG("generic set key: %s, fields: %zu", key.c_str(), entry.size());

    m_recorder->recordGenericSet(key, entry);

    m_communicationChannel->set(key, entry, REDIS_ASIC_STATE_COMMAND_SET);

    auto status = waitForResponse(OTAI_COMMON_API_SET);

    m_recorder->recordGenericSetResponse(status);

    return status;
}

otai_status_t RedisRemoteOtaiInterface::waitForResponse(
        _In_ otai_common_api_t api)
{
    SWSS_LOG_ENTER();

    if (m_syncMode)
    {
        swss::KeyOpFieldsValuesTuple kco;

        auto status = m_communicationChannel->wait(REDIS_ASIC_STATE_COMMAND_GETRESPONSE, kco);

        return status;
    }

    /*
     * By default sync mode is disabled and all create/set/remove are
     * considered success operations.
     */

    return OTAI_STATUS_SUCCESS;
}

otai_status_t RedisRemoteOtaiInterface::waitForGetResponse(
        _In_ otai_object_type_t objectType,
        _In_ uint32_t attr_count,
        _Inout_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    swss::KeyOpFieldsValuesTuple kco;

    auto status = m_communicationChannel->wait(REDIS_ASIC_STATE_COMMAND_GETRESPONSE, kco);

    auto &values = kfvFieldsValues(kco);

    if (status == OTAI_STATUS_SUCCESS)
    {
        if (values.size() == 0)
        {
            SWSS_LOG_THROW("logic error states = success, get response returned 0 values!, send api response or sync/async issue?");
        }

        OtaiAttributeList list(objectType, values, false);

        transfer_attributes(objectType, attr_count, list.get_attr_list(), attr_list, false);
    }
    else if (status == OTAI_STATUS_BUFFER_OVERFLOW)
    {
        if (values.size() == 0)
        {
            SWSS_LOG_THROW("logic error status = BUFFER_OVERFLOW, get response returned 0 values!, send api response or sync/async issue?");
        }

        OtaiAttributeList list(objectType, values, true);

        // no need for id fix since this is overflow
        transfer_attributes(objectType, attr_count, list.get_attr_list(), attr_list, true);
    }

    return status;
}

otai_status_t RedisRemoteOtaiInterface::get(
        _In_ otai_object_type_t objectType,
        _In_ const std::string& serializedObjectId,
        _In_ uint32_t attr_count,
        _Inout_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    /*
     * Since user may reuse buffers, then oid list buffers maybe not cleared
     * and contain some garbage, let's clean them so we send all oids as null to
     * syncd.
     */

    Utils::clearOidValues(objectType, attr_count, attr_list);

    auto entry = OtaiAttributeList::serialize_attr_list(objectType, attr_count, attr_list, false);

    std::string serializedObjectType = otai_serialize_object_type(objectType);

    std::string key = serializedObjectType + ":" + serializedObjectId;

    SWSS_LOG_DEBUG("generic get key: %s, fields: %lu", key.c_str(), entry.size());

    bool record = !m_skipRecordAttrContainer->canSkipRecording(objectType, attr_count, attr_list);

    if (record)
    {
        m_recorder->recordGenericGet(key, entry);
    }

    // get is special, it will not put data
    // into asic view, only to message queue
    m_communicationChannel->set(key, entry, REDIS_ASIC_STATE_COMMAND_GET);

    auto status = waitForGetResponse(objectType, attr_count, attr_list);

    if (record)
    {
        m_recorder->recordGenericGetResponse(status, objectType, attr_count, attr_list);
    }

    return status;
}


otai_status_t RedisRemoteOtaiInterface::getStats(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids,
        _Out_ otai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    auto stats_enum = otai_metadata_get_object_type_info(object_type)->statenum;

    auto entry = serialize_counter_id_list(stats_enum, number_of_counters, counter_ids);

    std::string str_object_type = otai_serialize_object_type(object_type);

    std::string key = str_object_type + ":" + otai_serialize_object_id(object_id);

    SWSS_LOG_DEBUG("generic get stats key: %s, fields: %zu", key.c_str(), entry.size());

    // get_stats will not put data to asic view, only to message queue

    m_communicationChannel->set(key, entry, REDIS_ASIC_STATE_COMMAND_GET_STATS);

    return waitForGetStatsResponse(object_type, number_of_counters, counter_ids, counters);
}

otai_status_t RedisRemoteOtaiInterface::waitForGetStatsResponse(
        _In_ otai_object_type_t object_type,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids,
        _Out_ otai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    swss::KeyOpFieldsValuesTuple kco;

    auto status = m_communicationChannel->wait(REDIS_ASIC_STATE_COMMAND_GETRESPONSE, kco);

    if (status == OTAI_STATUS_SUCCESS)
    {
        auto &values = kfvFieldsValues(kco);

        if (values.size () != number_of_counters)
        {
            SWSS_LOG_THROW("wrong number of counters, got %zu, expected %u", values.size(), number_of_counters);
        }

        for (uint32_t idx = 0; idx < number_of_counters; idx++)
        {
            auto stat_metadata  = otai_metadata_get_stat_metadata(object_type, counter_ids[idx]);
            if (stat_metadata->statvaluetype == OTAI_STAT_VALUE_TYPE_UINT64) { 
                counters[idx].u64 = stoull(fvValue(values[idx]));
            } else if (stat_metadata->statvaluetype == OTAI_STAT_VALUE_TYPE_DOUBLE) {
                counters[idx].d64 = stod(fvValue(values[idx]));
            }
        }
    }

    return status;
}

otai_status_t RedisRemoteOtaiInterface::getStatsExt(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids,
        _In_ otai_stats_mode_t mode,
        _Out_ otai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_ERROR("not implemented");

    // TODO could be the same as getStats but put mode at first argument

    return OTAI_STATUS_NOT_IMPLEMENTED;
}

otai_status_t RedisRemoteOtaiInterface::clearStats(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids)
{
    SWSS_LOG_ENTER();

    auto stats_enum = otai_metadata_get_object_type_info(object_type)->statenum;

    auto values = serialize_counter_id_list(stats_enum, number_of_counters, counter_ids);

    auto str_object_type = otai_serialize_object_type(object_type);

    auto key = str_object_type + ":" + otai_serialize_object_id(object_id);

    SWSS_LOG_DEBUG("generic clear stats key: %s, fields: %zu", key.c_str(), values.size());

    // clear_stats will not put data into asic view, only to message queue

    m_recorder->recordGenericClearStats(object_type, object_id, number_of_counters, counter_ids);

    m_communicationChannel->set(key, values, REDIS_ASIC_STATE_COMMAND_CLEAR_STATS);

    auto status = waitForClearStatsResponse();

    m_recorder->recordGenericClearStatsResponse(status);

    return status;
}

otai_status_t RedisRemoteOtaiInterface::waitForClearStatsResponse()
{
    SWSS_LOG_ENTER();

    swss::KeyOpFieldsValuesTuple kco;

    auto status = m_communicationChannel->wait(REDIS_ASIC_STATE_COMMAND_GETRESPONSE, kco);

    return status;
}

otai_status_t RedisRemoteOtaiInterface::notifySyncd(
        _In_ otai_object_id_t linecardId,
        _In_ otai_redis_notify_syncd_t redisNotifySyncd)
{
    SWSS_LOG_ENTER();

    std::vector<swss::FieldValueTuple> entry;

    auto key = otai_serialize(redisNotifySyncd);

    SWSS_LOG_NOTICE("sending syncd: %s", key.c_str());

    // we need to use "GET" channel to be sure that
    // all previous operations were applied, if we don't
    // use GET channel then we may hit race condition
    // on syncd side where syncd will start compare view
    // when there are still objects in op queue
    //
    // other solution can be to use notify event
    // and then on syncd side read all the asic state queue
    // and apply changes before linecarding to init/apply mode

    m_recorder->recordNotifySyncd(linecardId, redisNotifySyncd);

    m_communicationChannel->set(key, entry, REDIS_ASIC_STATE_COMMAND_NOTIFY);

    auto status = waitForNotifySyncdResponse();

    m_recorder->recordNotifySyncdResponse(status);

    return status;
}

otai_status_t RedisRemoteOtaiInterface::waitForNotifySyncdResponse()
{
    SWSS_LOG_ENTER();

    swss::KeyOpFieldsValuesTuple kco;

    auto status = m_communicationChannel->wait(REDIS_ASIC_STATE_COMMAND_NOTIFY, kco);

    return status;
}

bool RedisRemoteOtaiInterface::isRedisAttribute(
        _In_ otai_object_id_t objectType,
        _In_ const otai_attribute_t* attr)
{
    SWSS_LOG_ENTER();

    if ((objectType != OTAI_OBJECT_TYPE_LINECARD) || (attr == nullptr) || (attr->id < OTAI_LINECARD_ATTR_CUSTOM_RANGE_START))
    {
        return false;
    }

    return true;
}

void RedisRemoteOtaiInterface::handleNotification(
        _In_ const std::string &name,
        _In_ const std::string &serializedNotification,
        _In_ const std::vector<swss::FieldValueTuple> &values)
{
    SWSS_LOG_ENTER();

    // TODO to pass linecard_id for every notification we could add it to values
    // at syncd side
    //
    // Each global context (syncd) will have it's own notification thread
    // handler, so we will know at which context notification arrived, but we
    // also need to know at which linecard id generated this notification. For
    // that we will assign separate notification handlers in syncd itself, and
    // each of those notifications will know to which linecard id it belongs.
    // Then later we could also check whether oids in notification actually
    // belongs to given linecard id.  This way we could find vendor bugs like
    // sending notifications from one linecard to another linecard handler.
    //
    // But before that we will extract linecard id from notification itself.

    // TODO record should also be under api mutex, all other apis are

    m_recorder->recordNotification(name, serializedNotification, values);

    auto notification = NotificationFactory::deserialize(name, serializedNotification);

    if (notification)
    {
        auto sn = m_notificationCallback(notification); // will be synchronized to api mutex

        // execute callback from notification thread

        notification->executeCallback(sn);
    }
}

otai_object_type_t RedisRemoteOtaiInterface::objectTypeQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_virtualObjectIdManager->otaiObjectTypeQuery(objectId);
}

otai_object_id_t RedisRemoteOtaiInterface::linecardIdQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_virtualObjectIdManager->otaiLinecardIdQuery(objectId);
}

otai_status_t RedisRemoteOtaiInterface::logSet(
        _In_ otai_api_t api,
        _In_ otai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_SUCCESS;
}

otai_status_t RedisRemoteOtaiInterface::otai_redis_notify_syncd(
        _In_ otai_object_id_t linecardId,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    auto redisNotifySyncd = (otai_redis_notify_syncd_t)attr->value.s32;

    switch (redisNotifySyncd)
    {
        case OTAI_REDIS_NOTIFY_SYNCD_INIT_VIEW:
        case OTAI_REDIS_NOTIFY_SYNCD_APPLY_VIEW:
        case OTAI_REDIS_NOTIFY_SYNCD_INSPECT_ASIC:
            break;

        default:

            SWSS_LOG_ERROR("invalid notify syncd attr value %s", otai_serialize(redisNotifySyncd).c_str());

            return OTAI_STATUS_FAILURE;
    }

    auto status = notifySyncd(linecardId, redisNotifySyncd);

    if (status == OTAI_STATUS_SUCCESS)
    {
        switch (redisNotifySyncd)
        {
            case OTAI_REDIS_NOTIFY_SYNCD_INIT_VIEW:

                SWSS_LOG_NOTICE("linecarded ASIC to INIT VIEW");

                m_asicInitViewMode = true;

                SWSS_LOG_NOTICE("clearing current local state since init view is called on initialized linecard");

                clear_local_state();

                break;

            case OTAI_REDIS_NOTIFY_SYNCD_APPLY_VIEW:

                SWSS_LOG_NOTICE("linecarded ASIC to APPLY VIEW");

                m_asicInitViewMode = false;

                break;

            case OTAI_REDIS_NOTIFY_SYNCD_INSPECT_ASIC:

                SWSS_LOG_NOTICE("inspect ASIC SUCCEEDED");

                break;

            default:
                break;
        }
    }

    return status;
}

void RedisRemoteOtaiInterface::clear_local_state()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("clearing local state");

    // Will need to be executed after init VIEW

    // will clear linecard container
    m_linecardContainer = std::make_shared<LinecardContainer>();

    m_virtualObjectIdManager = 
        std::make_shared<VirtualObjectIdManager>(
                m_contextConfig->m_guid,
                m_contextConfig->m_scc,
                m_redisVidIndexGenerator);

    auto meta = m_meta.lock();

    if (meta)
    {
        meta->meta_init_db();
    }
}

void RedisRemoteOtaiInterface::setMeta(
        _In_ std::weak_ptr<otaimeta::Meta> meta)
{
    SWSS_LOG_ENTER();

    m_meta = meta;
}

otai_linecard_notifications_t RedisRemoteOtaiInterface::syncProcessNotification(
        _In_ std::shared_ptr<Notification> notification)
{
    SWSS_LOG_ENTER();

    // NOTE: process metadata must be executed under otairedis API mutex since
    // it will access meta database and notification comes from different
    // thread, and this method is executed from notifications thread

    auto meta = m_meta.lock();

    if (!meta)
    {
        SWSS_LOG_WARN("meta pointer expired");

        return {nullptr, nullptr, nullptr, nullptr};
    }

    notification->processMetadata(meta);

    auto objectId = notification->getAnyObjectId();

    auto linecardId = m_virtualObjectIdManager->otaiLinecardIdQuery(objectId);

    auto sw = m_linecardContainer->getLinecard(linecardId);

    if (sw)
    {
        return sw->getLinecardNotifications(); // explicit copy
    }

    SWSS_LOG_WARN("linecard %s not present in container, returning empty linecard notifications",
            otai_serialize_object_id(linecardId).c_str());

    return {nullptr, nullptr, nullptr, nullptr};
}

