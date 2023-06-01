#include "RedisRemoteLaiInterface.h"
#include "Utils.h"
#include "NotificationFactory.h"
#include "Recorder.h"
#include "VirtualObjectIdManager.h"
#include "SkipRecordAttrContainer.h"
#include "LinecardContainer.h"
#include "PerformanceIntervalTimer.h"

#include "lairediscommon.h"

#include "meta/lai_serialize.h"
#include "meta/LaiAttributeList.h"

#include <inttypes.h>

using namespace lairedis;
using namespace laimeta;
using namespace lairediscommon;
using namespace std::placeholders;

std::string joinFieldValues(
        _In_ const std::vector<swss::FieldValueTuple> &values);

std::vector<swss::FieldValueTuple> serialize_counter_id_list(
        _In_ const lai_enum_metadata_t *stats_enum,
        _In_ uint32_t count,
        _In_ const lai_stat_id_t *counter_id_list);

std::vector<swss::FieldValueTuple> serialize_alarm_id_list(
        _In_ const lai_enum_metadata_t *alarms_enum,
        _In_ uint32_t number_of_alarms,
        _In_ const lai_alarm_type_t *alarm_id_list);

RedisRemoteLaiInterface::RedisRemoteLaiInterface(
        _In_ std::shared_ptr<ContextConfig> contextConfig,
        _In_ std::function<lai_linecard_notifications_t(std::shared_ptr<Notification>)> notificationCallback,
        _In_ std::shared_ptr<Recorder> recorder):
    m_contextConfig(contextConfig),
    m_redisCommunicationMode(LAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC),
    m_recorder(recorder),
    m_notificationCallback(notificationCallback)
{
    SWSS_LOG_ENTER();

    m_initialized = false;

    initialize(0, nullptr);
}

RedisRemoteLaiInterface::~RedisRemoteLaiInterface()
{
    SWSS_LOG_ENTER();

    if (m_initialized)
    {
        uninitialize();
    }
}

lai_status_t RedisRemoteLaiInterface::initialize(
        _In_ uint64_t flags,
        _In_ const lai_service_method_table_t *service_method_table)
{
    SWSS_LOG_ENTER();

    if (m_initialized)
    {
        SWSS_LOG_ERROR("already initialized");

        return LAI_STATUS_FAILURE;
    }

    m_skipRecordAttrContainer = std::make_shared<SkipRecordAttrContainer>();

    m_asicInitViewMode = false; // default mode is apply mode
    m_useTempView = false;
    m_syncMode = false;
    m_redisCommunicationMode = LAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC;

    m_communicationChannel = std::make_shared<RedisChannel>(
            m_contextConfig->m_dbAsic,
            std::bind(&RedisRemoteLaiInterface::handleNotification, this, _1, _2, _3));

    m_responseTimeoutMs = m_communicationChannel->getResponseTimeout();

    m_db = std::make_shared<swss::DBConnector>(m_contextConfig->m_dbAsic, 0);

    m_redisVidIndexGenerator = std::make_shared<RedisVidIndexGenerator>(m_db, REDIS_KEY_VIDCOUNTER);

    clear_local_state();

    // TODO what will happen when we receive notification in init view mode ?

    m_initialized = true;

    return LAI_STATUS_SUCCESS;
}

lai_status_t RedisRemoteLaiInterface::uninitialize(void)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("begin");

    if (!m_initialized)
    {
        SWSS_LOG_ERROR("not initialized");

        return LAI_STATUS_FAILURE;
    }

    m_communicationChannel = nullptr; // will stop thread

    // clear local state after stopping threads

    clear_local_state();

    m_initialized = false;

    SWSS_LOG_NOTICE("end");

    return LAI_STATUS_SUCCESS;
}

lai_status_t RedisRemoteLaiInterface::linkCheck(_Out_ bool *up)
{
    return LAI_STATUS_SUCCESS;
}

std::string RedisRemoteLaiInterface::getHardwareInfo(
        _In_ uint32_t attrCount,
        _In_ const lai_attribute_t *attrList) const
{
    SWSS_LOG_ENTER();

    return "";
}


lai_status_t RedisRemoteLaiInterface::create(
        _In_ lai_object_type_t objectType,
        _Out_ lai_object_id_t* objectId,
        _In_ lai_object_id_t linecardId,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    *objectId = LAI_NULL_OBJECT_ID;

    if (objectType == LAI_OBJECT_TYPE_LINECARD)
    {
        // for given hardware info we always return same linecard id,
        // this is required since we could be performing warm boot here

        auto hwinfo = getHardwareInfo(attr_count, attr_list);

        linecardId = m_virtualObjectIdManager->allocateNewLinecardObjectId(hwinfo);

        *objectId = linecardId;

        if (linecardId == LAI_NULL_OBJECT_ID)
        {
            SWSS_LOG_ERROR("linecard ID allocation failed");

            return LAI_STATUS_FAILURE;
        }
    }
    else
    {
        *objectId = m_virtualObjectIdManager->allocateNewObjectId(objectType, linecardId);
    }

    if (*objectId == LAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_ERROR("failed to create %s, with linecard id: %s",
                lai_serialize_object_type(objectType).c_str(),
                lai_serialize_object_id(linecardId).c_str());

        return LAI_STATUS_INSUFFICIENT_RESOURCES;
    }

    // NOTE: objectId was allocated by the caller

    auto status = create(
            objectType,
            lai_serialize_object_id(*objectId),
            attr_count,
            attr_list);

    if (objectType == LAI_OBJECT_TYPE_LINECARD && status == LAI_STATUS_SUCCESS)
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
    else if (status != LAI_STATUS_SUCCESS)
    {
        // if create failed, then release allocated object
        m_virtualObjectIdManager->releaseObjectId(*objectId);
    }

    return status;
}

lai_status_t RedisRemoteLaiInterface::remove(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    auto status = remove(
            objectType,
            lai_serialize_object_id(objectId));

    if (objectType == LAI_OBJECT_TYPE_LINECARD && status == LAI_STATUS_SUCCESS)
    {
        SWSS_LOG_NOTICE("removing linecard id %s", lai_serialize_object_id(objectId).c_str());

        m_virtualObjectIdManager->releaseObjectId(objectId);

        // remove linecard from container
        m_linecardContainer->removeLinecard(objectId);
    }

    return status;
}

lai_status_t RedisRemoteLaiInterface::setRedisExtensionAttribute(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    if (attr == nullptr)
    {
        SWSS_LOG_ERROR("attr pointer is null");

        return LAI_STATUS_FAILURE;
    }

    /*
     * NOTE: that this will work without
     * linecard being created.
     */

    switch (attr->id)
    {
        case LAI_REDIS_LINECARD_ATTR_PERFORM_LOG_ROTATE:

            if (m_recorder)
            {
                m_recorder->requestLogRotate();
            }

            return LAI_STATUS_SUCCESS;

        case LAI_REDIS_LINECARD_ATTR_RECORD:

            if (m_recorder)
            {
                m_recorder->enableRecording(attr->value.booldata);
            }

            return LAI_STATUS_SUCCESS;

        case LAI_REDIS_LINECARD_ATTR_NOTIFY_SYNCD:

            return lai_redis_notify_syncd(objectId, attr);

        case LAI_REDIS_LINECARD_ATTR_USE_TEMP_VIEW:

            m_useTempView = attr->value.booldata;

            return LAI_STATUS_SUCCESS;

        case LAI_REDIS_LINECARD_ATTR_RECORD_STATS:

            m_recorder->recordStats(attr->value.booldata);

            return LAI_STATUS_SUCCESS;

        case LAI_REDIS_LINECARD_ATTR_SYNC_OPERATION_RESPONSE_TIMEOUT:

            m_responseTimeoutMs = attr->value.u64;

            m_communicationChannel->setResponseTimeout(m_responseTimeoutMs);

            SWSS_LOG_NOTICE("set response timeout to %" PRIu64 " ms", m_responseTimeoutMs);

            return LAI_STATUS_SUCCESS;

        case LAI_REDIS_LINECARD_ATTR_SYNC_MODE:

            SWSS_LOG_WARN("sync mode is depreacated, use communication mode");

            m_syncMode = attr->value.booldata;

            if (m_syncMode)
            {
                SWSS_LOG_NOTICE("disabling buffered pipeline in sync mode");

                m_communicationChannel->setBuffered(false);
            }

            return LAI_STATUS_SUCCESS;

        case LAI_REDIS_LINECARD_ATTR_REDIS_COMMUNICATION_MODE:

            m_redisCommunicationMode = (lai_redis_communication_mode_t)attr->value.s32;

            m_communicationChannel = nullptr;

            switch (m_redisCommunicationMode)
            {
                case LAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC:

                    SWSS_LOG_NOTICE("enabling redis async mode");

                    m_syncMode = false;

                    m_communicationChannel = std::make_shared<RedisChannel>(
                            m_contextConfig->m_dbAsic,
                            std::bind(&RedisRemoteLaiInterface::handleNotification, this, _1, _2, _3));

                    m_communicationChannel->setResponseTimeout(m_responseTimeoutMs);

                    m_communicationChannel->setBuffered(true);

                    return LAI_STATUS_SUCCESS;

                case LAI_REDIS_COMMUNICATION_MODE_REDIS_SYNC:

                    SWSS_LOG_NOTICE("enabling redis sync mode");

                    m_syncMode = true;

                    m_communicationChannel = std::make_shared<RedisChannel>(
                            m_contextConfig->m_dbAsic,
                            std::bind(&RedisRemoteLaiInterface::handleNotification, this, _1, _2, _3));

                    m_communicationChannel->setResponseTimeout(m_responseTimeoutMs);

                    m_communicationChannel->setBuffered(false);

                    return LAI_STATUS_SUCCESS;

                default:

                    SWSS_LOG_ERROR("invalid communication mode value: %d", m_redisCommunicationMode);

                    return LAI_STATUS_NOT_SUPPORTED;
            }

        case LAI_REDIS_LINECARD_ATTR_USE_PIPELINE:

            if (m_syncMode)
            {
                SWSS_LOG_WARN("use pipeline is not supported in sync mode");

                return LAI_STATUS_NOT_SUPPORTED;
            }

            m_communicationChannel->setBuffered(attr->value.booldata);

            return LAI_STATUS_SUCCESS;

        case LAI_REDIS_LINECARD_ATTR_FLUSH:

            m_communicationChannel->flush();

            return LAI_STATUS_SUCCESS;

        case LAI_REDIS_LINECARD_ATTR_RECORDING_OUTPUT_DIR:

            if (m_recorder)
            {
                m_recorder->setRecordingOutputDirectory(*attr);
            }

            return LAI_STATUS_SUCCESS;

        case LAI_REDIS_LINECARD_ATTR_RECORDING_FILENAME:

            if (m_recorder)
            {
                m_recorder->setRecordingFilename(*attr);
            }

            return LAI_STATUS_SUCCESS;
            
        default:
            break;
    }

    SWSS_LOG_ERROR("unknown redis extension attribute: %d", attr->id);

    return LAI_STATUS_FAILURE;
}

lai_status_t RedisRemoteLaiInterface::set(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    if (RedisRemoteLaiInterface::isRedisAttribute(objectType, attr))
    {
        return setRedisExtensionAttribute(objectType, objectId, attr);
    }

    auto status = set(
            objectType,
            lai_serialize_object_id(objectId),
            attr);

    if (objectType == LAI_OBJECT_TYPE_LINECARD && status == LAI_STATUS_SUCCESS)
    {
        auto sw = m_linecardContainer->getLinecard(objectId);

        if (!sw)
        {
            SWSS_LOG_THROW("failed to find linecard %s in container",
                    lai_serialize_object_id(objectId).c_str());
        }

        /*
         * When doing SET operation user may want to update notification
         * pointers.
         */

        sw->updateNotifications(1, attr);
    }

    return status;
}

lai_status_t RedisRemoteLaiInterface::get(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId,
        _In_ uint32_t attr_count,
        _Inout_ lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return get(
            objectType,
            lai_serialize_object_id(objectId),
            attr_count,
            attr_list);
}

lai_status_t RedisRemoteLaiInterface::create(
        _In_ lai_object_type_t object_type,
        _In_ const std::string& serializedObjectId,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    auto entry = LaiAttributeList::serialize_attr_list(
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

    auto serializedObjectType = lai_serialize_object_type(object_type);

    const std::string key = serializedObjectType + ":" + serializedObjectId;

    SWSS_LOG_NOTICE("generic create key: %s, fields: %zu", key.c_str(), entry.size());

    m_recorder->recordGenericCreate(key, entry);

    m_communicationChannel->set(key, entry, REDIS_ASIC_STATE_COMMAND_CREATE);

    auto status = waitForResponse(LAI_COMMON_API_CREATE);
    SWSS_LOG_NOTICE("generic create key end: %s, fields: %zu", key.c_str(), entry.size());

    m_recorder->recordGenericCreateResponse(status);

    return status;
}

lai_status_t RedisRemoteLaiInterface::remove(
        _In_ lai_object_type_t objectType,
        _In_ const std::string& serializedObjectId)
{
    SWSS_LOG_ENTER();

    auto serializedObjectType = lai_serialize_object_type(objectType);

    const std::string key = serializedObjectType + ":" + serializedObjectId;

    SWSS_LOG_NOTICE("generic remove key: %s", key.c_str());

    m_recorder->recordGenericRemove(key);

    m_communicationChannel->del(key, REDIS_ASIC_STATE_COMMAND_REMOVE);

    auto status = waitForResponse(LAI_COMMON_API_REMOVE);

    m_recorder->recordGenericRemoveResponse(status);

    return status;
}

lai_status_t RedisRemoteLaiInterface::set(
        _In_ lai_object_type_t objectType,
        _In_ const std::string &serializedObjectId,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    auto entry = LaiAttributeList::serialize_attr_list(
            objectType,
            1,
            attr,
            false);

    auto serializedObjectType = lai_serialize_object_type(objectType);

    std::string key = serializedObjectType + ":" + serializedObjectId;

    SWSS_LOG_DEBUG("generic set key: %s, fields: %zu", key.c_str(), entry.size());

    m_recorder->recordGenericSet(key, entry);

    m_communicationChannel->set(key, entry, REDIS_ASIC_STATE_COMMAND_SET);

    auto status = waitForResponse(LAI_COMMON_API_SET);

    m_recorder->recordGenericSetResponse(status);

    return status;
}

lai_status_t RedisRemoteLaiInterface::waitForResponse(
        _In_ lai_common_api_t api)
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

    return LAI_STATUS_SUCCESS;
}

lai_status_t RedisRemoteLaiInterface::waitForGetResponse(
        _In_ lai_object_type_t objectType,
        _In_ uint32_t attr_count,
        _Inout_ lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    swss::KeyOpFieldsValuesTuple kco;

    auto status = m_communicationChannel->wait(REDIS_ASIC_STATE_COMMAND_GETRESPONSE, kco);

    auto &values = kfvFieldsValues(kco);

    if (status == LAI_STATUS_SUCCESS)
    {
        if (values.size() == 0)
        {
            SWSS_LOG_THROW("logic error states = success, get response returned 0 values!, send api response or sync/async issue?");
        }

        LaiAttributeList list(objectType, values, false);

        transfer_attributes(objectType, attr_count, list.get_attr_list(), attr_list, false);
    }
    else if (status == LAI_STATUS_BUFFER_OVERFLOW)
    {
        if (values.size() == 0)
        {
            SWSS_LOG_THROW("logic error status = BUFFER_OVERFLOW, get response returned 0 values!, send api response or sync/async issue?");
        }

        LaiAttributeList list(objectType, values, true);

        // no need for id fix since this is overflow
        transfer_attributes(objectType, attr_count, list.get_attr_list(), attr_list, true);
    }

    return status;
}

lai_status_t RedisRemoteLaiInterface::get(
        _In_ lai_object_type_t objectType,
        _In_ const std::string& serializedObjectId,
        _In_ uint32_t attr_count,
        _Inout_ lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    /*
     * Since user may reuse buffers, then oid list buffers maybe not cleared
     * and contain some garbage, let's clean them so we send all oids as null to
     * syncd.
     */

    Utils::clearOidValues(objectType, attr_count, attr_list);

    auto entry = LaiAttributeList::serialize_attr_list(objectType, attr_count, attr_list, false);

    std::string serializedObjectType = lai_serialize_object_type(objectType);

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


lai_status_t RedisRemoteLaiInterface::getStats(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids,
        _Out_ lai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    auto stats_enum = lai_metadata_get_object_type_info(object_type)->statenum;

    auto entry = serialize_counter_id_list(stats_enum, number_of_counters, counter_ids);

    std::string str_object_type = lai_serialize_object_type(object_type);

    std::string key = str_object_type + ":" + lai_serialize_object_id(object_id);

    SWSS_LOG_DEBUG("generic get stats key: %s, fields: %zu", key.c_str(), entry.size());

    // get_stats will not put data to asic view, only to message queue

    m_communicationChannel->set(key, entry, REDIS_ASIC_STATE_COMMAND_GET_STATS);

    return waitForGetStatsResponse(object_type, number_of_counters, counter_ids, counters);
}

lai_status_t RedisRemoteLaiInterface::waitForGetStatsResponse(
        _In_ lai_object_type_t object_type,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids,
        _Out_ lai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    swss::KeyOpFieldsValuesTuple kco;

    auto status = m_communicationChannel->wait(REDIS_ASIC_STATE_COMMAND_GETRESPONSE, kco);

    if (status == LAI_STATUS_SUCCESS)
    {
        auto &values = kfvFieldsValues(kco);

        if (values.size () != number_of_counters)
        {
            SWSS_LOG_THROW("wrong number of counters, got %zu, expected %u", values.size(), number_of_counters);
        }

        for (uint32_t idx = 0; idx < number_of_counters; idx++)
        {
            auto stat_metadata  = lai_metadata_get_stat_metadata(object_type, counter_ids[idx]);
            if (stat_metadata->statvaluetype == LAI_STAT_VALUE_TYPE_UINT64) { 
                counters[idx].u64 = stoull(fvValue(values[idx]));
            } else if (stat_metadata->statvaluetype == LAI_STAT_VALUE_TYPE_DOUBLE) {
                counters[idx].d64 = stod(fvValue(values[idx]));
            }
        }
    }

    return status;
}

lai_status_t RedisRemoteLaiInterface::getStatsExt(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids,
        _In_ lai_stats_mode_t mode,
        _Out_ lai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_ERROR("not implemented");

    // TODO could be the same as getStats but put mode at first argument

    return LAI_STATUS_NOT_IMPLEMENTED;
}

lai_status_t RedisRemoteLaiInterface::clearStats(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids)
{
    SWSS_LOG_ENTER();

    auto stats_enum = lai_metadata_get_object_type_info(object_type)->statenum;

    auto values = serialize_counter_id_list(stats_enum, number_of_counters, counter_ids);

    auto str_object_type = lai_serialize_object_type(object_type);

    auto key = str_object_type + ":" + lai_serialize_object_id(object_id);

    SWSS_LOG_DEBUG("generic clear stats key: %s, fields: %zu", key.c_str(), values.size());

    // clear_stats will not put data into asic view, only to message queue

    m_recorder->recordGenericClearStats(object_type, object_id, number_of_counters, counter_ids);

    m_communicationChannel->set(key, values, REDIS_ASIC_STATE_COMMAND_CLEAR_STATS);

    auto status = waitForClearStatsResponse();

    m_recorder->recordGenericClearStatsResponse(status);

    return status;
}

lai_status_t RedisRemoteLaiInterface::waitForClearStatsResponse()
{
    SWSS_LOG_ENTER();

    swss::KeyOpFieldsValuesTuple kco;

    auto status = m_communicationChannel->wait(REDIS_ASIC_STATE_COMMAND_GETRESPONSE, kco);

    return status;
}

lai_status_t RedisRemoteLaiInterface::objectTypeGetAvailability(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ uint32_t attrCount,
        _In_ const lai_attribute_t *attrList,
        _Out_ uint64_t *count)
{
    SWSS_LOG_ENTER();

    auto strLinecardId = lai_serialize_object_id(linecardId);

    auto entry = LaiAttributeList::serialize_attr_list(objectType, attrCount, attrList, false);

    entry.push_back(swss::FieldValueTuple("OBJECT_TYPE", lai_serialize_object_type(objectType)));

    SWSS_LOG_DEBUG(
            "Query arguments: linecard: %s, attributes: %s",
            strLinecardId.c_str(),
            joinFieldValues(entry).c_str());

    // Syncd will pop this argument off before trying to deserialize the attribute list

    m_recorder->recordObjectTypeGetAvailability(linecardId, objectType, attrCount, attrList);
    // recordObjectTypeGetAvailability(strLinecardId, entry);

    // This query will not put any data into the ASIC view, just into the
    // message queue
    m_communicationChannel->set(strLinecardId, entry, REDIS_ASIC_STATE_COMMAND_OBJECT_TYPE_GET_AVAILABILITY_QUERY);

    auto status = waitForObjectTypeGetAvailabilityResponse(count);

    m_recorder->recordObjectTypeGetAvailabilityResponse(status, count);

    return status;
}

lai_status_t RedisRemoteLaiInterface::waitForObjectTypeGetAvailabilityResponse(
        _Inout_ uint64_t *count)
{
    SWSS_LOG_ENTER();

    swss::KeyOpFieldsValuesTuple kco;

    auto status = m_communicationChannel->wait(REDIS_ASIC_STATE_COMMAND_OBJECT_TYPE_GET_AVAILABILITY_RESPONSE, kco);

    if (status == LAI_STATUS_SUCCESS)
    {
        auto &values = kfvFieldsValues(kco);

        if (values.size() != 1)
        {
            SWSS_LOG_THROW("Invalid response from syncd: expected 1 value, received %zu", values.size());
        }

        const std::string &availability_str = fvValue(values[0]);

        *count = std::stol(availability_str);

        SWSS_LOG_DEBUG("Received payload: count = %" PRIu64, *count);
    }

    return status;
}

lai_status_t RedisRemoteLaiInterface::queryAttributeCapability(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ lai_attr_id_t attrId,
        _Out_ lai_attr_capability_t *capability)
{
    SWSS_LOG_ENTER();

    auto linecardIdStr = lai_serialize_object_id(linecardId);
    auto objectTypeStr = lai_serialize_object_type(objectType);

    auto meta = lai_metadata_get_attr_metadata(objectType, attrId);

    if (meta == NULL)
    {
        SWSS_LOG_ERROR("Failed to find attribute metadata: object type %s, attr id %d", objectTypeStr.c_str(), attrId);
        return LAI_STATUS_INVALID_PARAMETER;
    }

    const std::string attrIdStr = meta->attridname;

    const std::vector<swss::FieldValueTuple> entry =
    {
        swss::FieldValueTuple("OBJECT_TYPE", objectTypeStr),
        swss::FieldValueTuple("ATTR_ID", attrIdStr)
    };

    SWSS_LOG_DEBUG(
            "Query arguments: linecard %s, object type: %s, attribute: %s",
            linecardIdStr.c_str(),
            objectTypeStr.c_str(),
            attrIdStr.c_str()
    );

    // This query will not put any data into the ASIC view, just into the
    // message queue

    m_recorder->recordQueryAttributeCapability(linecardId, objectType, attrId, capability);

    m_communicationChannel->set(linecardIdStr, entry, REDIS_ASIC_STATE_COMMAND_ATTR_CAPABILITY_QUERY);

    auto status = waitForQueryAttributeCapabilityResponse(capability);

    m_recorder->recordQueryAttributeCapabilityResponse(status, objectType, attrId, capability);

    return status;
}

lai_status_t RedisRemoteLaiInterface::waitForQueryAttributeCapabilityResponse(
        _Out_ lai_attr_capability_t* capability)
{
    SWSS_LOG_ENTER();

    swss::KeyOpFieldsValuesTuple kco;

    auto status = m_communicationChannel->wait(REDIS_ASIC_STATE_COMMAND_ATTR_CAPABILITY_RESPONSE, kco);

    if (status == LAI_STATUS_SUCCESS)
    {
        const std::vector<swss::FieldValueTuple> &values = kfvFieldsValues(kco);

        if (values.size() != 3)
        {
            SWSS_LOG_ERROR("Invalid response from syncd: expected 3 values, received %zu", values.size());

            return LAI_STATUS_FAILURE;
        }

        capability->create_implemented = (fvValue(values[0]) == "true" ? true : false);
        capability->set_implemented    = (fvValue(values[1]) == "true" ? true : false);
        capability->get_implemented    = (fvValue(values[2]) == "true" ? true : false);

        SWSS_LOG_DEBUG("Received payload: create_implemented:%s, set_implemented:%s, get_implemented:%s",
            (capability->create_implemented? "true":"false"), (capability->set_implemented? "true":"false"), (capability->get_implemented? "true":"false"));
    }

    return status;
}

lai_status_t RedisRemoteLaiInterface::queryAattributeEnumValuesCapability(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ lai_attr_id_t attrId,
        _Inout_ lai_s32_list_t *enumValuesCapability)
{
    SWSS_LOG_ENTER();

    if (enumValuesCapability && enumValuesCapability->list)
    {
        // clear input list, since we use serialize to transfer values
        for (uint32_t idx = 0; idx < enumValuesCapability->count; idx++)
            enumValuesCapability->list[idx] = 0;
    }

    auto linecard_id_str = lai_serialize_object_id(linecardId);
    auto object_type_str = lai_serialize_object_type(objectType);

    auto meta = lai_metadata_get_attr_metadata(objectType, attrId);

    if (meta == NULL)
    {
        SWSS_LOG_ERROR("Failed to find attribute metadata: object type %s, attr id %d", object_type_str.c_str(), attrId);
        return LAI_STATUS_INVALID_PARAMETER;
    }

    const std::string attr_id_str = meta->attridname;
    const std::string list_size = std::to_string(enumValuesCapability->count);

    const std::vector<swss::FieldValueTuple> entry =
    {
        swss::FieldValueTuple("OBJECT_TYPE", object_type_str),
        swss::FieldValueTuple("ATTR_ID", attr_id_str),
        swss::FieldValueTuple("LIST_SIZE", list_size)
    };

    SWSS_LOG_DEBUG(
            "Query arguments: linecard %s, object type: %s, attribute: %s, count: %s",
            linecard_id_str.c_str(),
            object_type_str.c_str(),
            attr_id_str.c_str(),
            list_size.c_str()
    );

    // This query will not put any data into the ASIC view, just into the
    // message queue

    m_recorder->recordQueryAattributeEnumValuesCapability(linecardId, objectType, attrId, enumValuesCapability);

    m_communicationChannel->set(linecard_id_str, entry, REDIS_ASIC_STATE_COMMAND_ATTR_ENUM_VALUES_CAPABILITY_QUERY);

    auto status = waitForQueryAattributeEnumValuesCapabilityResponse(enumValuesCapability);

    m_recorder->recordQueryAattributeEnumValuesCapabilityResponse(status, objectType, attrId, enumValuesCapability);

    return status;
}

lai_status_t RedisRemoteLaiInterface::waitForQueryAattributeEnumValuesCapabilityResponse(
        _Inout_ lai_s32_list_t* enumValuesCapability)
{
    SWSS_LOG_ENTER();

    swss::KeyOpFieldsValuesTuple kco;

    auto status = m_communicationChannel->wait(REDIS_ASIC_STATE_COMMAND_ATTR_ENUM_VALUES_CAPABILITY_RESPONSE, kco);

    if (status == LAI_STATUS_SUCCESS)
    {
        const std::vector<swss::FieldValueTuple> &values = kfvFieldsValues(kco);

        if (values.size() != 2)
        {
            SWSS_LOG_ERROR("Invalid response from syncd: expected 2 values, received %zu", values.size());

            return LAI_STATUS_FAILURE;
        }

        const std::string &capability_str = fvValue(values[0]);
        const uint32_t num_capabilities = std::stoi(fvValue(values[1]));

        SWSS_LOG_DEBUG("Received payload: capabilities = '%s', count = %d", capability_str.c_str(), num_capabilities);

        enumValuesCapability->count = num_capabilities;

        size_t position = 0;
        for (uint32_t i = 0; i < num_capabilities; i++)
        {
            size_t old_position = position;
            position = capability_str.find(",", old_position);
            std::string capability = capability_str.substr(old_position, position - old_position);
            enumValuesCapability->list[i] = std::stoi(capability);

            // We have run out of values to add to our list
            if (position == std::string::npos)
            {
                if (num_capabilities != i + 1)
                {
                    SWSS_LOG_WARN("Query returned less attributes than expected: expected %d, received %d", num_capabilities, i+1);
                }

                break;
            }

            // Skip the commas
            position++;
        }
    }
    else if (status ==  LAI_STATUS_BUFFER_OVERFLOW)
    {
        // TODO on lai status overflow we should populate correct count on the list

        SWSS_LOG_ERROR("TODO need to handle LAI_STATUS_BUFFER_OVERFLOW, FIXME");
    }

    return status;
}

lai_status_t RedisRemoteLaiInterface::notifySyncd(
        _In_ lai_object_id_t linecardId,
        _In_ lai_redis_notify_syncd_t redisNotifySyncd)
{
    SWSS_LOG_ENTER();

    std::vector<swss::FieldValueTuple> entry;

    auto key = lai_serialize(redisNotifySyncd);

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

lai_status_t RedisRemoteLaiInterface::waitForNotifySyncdResponse()
{
    SWSS_LOG_ENTER();

    swss::KeyOpFieldsValuesTuple kco;

    auto status = m_communicationChannel->wait(REDIS_ASIC_STATE_COMMAND_NOTIFY, kco);

    return status;
}

bool RedisRemoteLaiInterface::isRedisAttribute(
        _In_ lai_object_id_t objectType,
        _In_ const lai_attribute_t* attr)
{
    SWSS_LOG_ENTER();

    if ((objectType != LAI_OBJECT_TYPE_LINECARD) || (attr == nullptr) || (attr->id < LAI_LINECARD_ATTR_CUSTOM_RANGE_START))
    {
        return false;
    }

    return true;
}

void RedisRemoteLaiInterface::handleNotification(
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

lai_object_type_t RedisRemoteLaiInterface::objectTypeQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_virtualObjectIdManager->laiObjectTypeQuery(objectId);
}

lai_object_id_t RedisRemoteLaiInterface::linecardIdQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_virtualObjectIdManager->laiLinecardIdQuery(objectId);
}

lai_status_t RedisRemoteLaiInterface::logSet(
        _In_ lai_api_t api,
        _In_ lai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_SUCCESS;
}

lai_status_t RedisRemoteLaiInterface::lai_redis_notify_syncd(
        _In_ lai_object_id_t linecardId,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    auto redisNotifySyncd = (lai_redis_notify_syncd_t)attr->value.s32;

    switch (redisNotifySyncd)
    {
        case LAI_REDIS_NOTIFY_SYNCD_INIT_VIEW:
        case LAI_REDIS_NOTIFY_SYNCD_APPLY_VIEW:
        case LAI_REDIS_NOTIFY_SYNCD_INSPECT_ASIC:
            break;

        default:

            SWSS_LOG_ERROR("invalid notify syncd attr value %s", lai_serialize(redisNotifySyncd).c_str());

            return LAI_STATUS_FAILURE;
    }

    auto status = notifySyncd(linecardId, redisNotifySyncd);

    if (status == LAI_STATUS_SUCCESS)
    {
        switch (redisNotifySyncd)
        {
            case LAI_REDIS_NOTIFY_SYNCD_INIT_VIEW:

                SWSS_LOG_NOTICE("linecarded ASIC to INIT VIEW");

                m_asicInitViewMode = true;

                SWSS_LOG_NOTICE("clearing current local state since init view is called on initialized linecard");

                clear_local_state();

                break;

            case LAI_REDIS_NOTIFY_SYNCD_APPLY_VIEW:

                SWSS_LOG_NOTICE("linecarded ASIC to APPLY VIEW");

                m_asicInitViewMode = false;

                break;

            case LAI_REDIS_NOTIFY_SYNCD_INSPECT_ASIC:

                SWSS_LOG_NOTICE("inspect ASIC SUCCEEDED");

                break;

            default:
                break;
        }
    }

    return status;
}

void RedisRemoteLaiInterface::clear_local_state()
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

void RedisRemoteLaiInterface::setMeta(
        _In_ std::weak_ptr<laimeta::Meta> meta)
{
    SWSS_LOG_ENTER();

    m_meta = meta;
}

lai_linecard_notifications_t RedisRemoteLaiInterface::syncProcessNotification(
        _In_ std::shared_ptr<Notification> notification)
{
    SWSS_LOG_ENTER();

    // NOTE: process metadata must be executed under lairedis API mutex since
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

    auto linecardId = m_virtualObjectIdManager->laiLinecardIdQuery(objectId);

    auto sw = m_linecardContainer->getLinecard(linecardId);

    if (sw)
    {
        return sw->getLinecardNotifications(); // explicit copy
    }

    SWSS_LOG_WARN("linecard %s not present in container, returning empty linecard notifications",
            lai_serialize_object_id(linecardId).c_str());

    return {nullptr, nullptr, nullptr, nullptr};
}

