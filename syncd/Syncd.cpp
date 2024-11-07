#include "Syncd.h"
#include "VidManager.h"
#include "NotificationHandler.h"
#include "HardReiniter.h"
#include "SoftReiniter.h"
#include "RedisClient.h"
#include "RequestShutdown.h"
#include "ContextConfigContainer.h"
#include "RedisNotificationProducer.h"
#include "RedisSelectableChannel.h"

#include "otairediscommon.h"

#include "swss/logger.h"
#include "swss/select.h"
#include "swss/tokenize.h"
#include "swss/notificationproducer.h"

#include "meta/otai_serialize.h"

#include <unistd.h>
#include <inttypes.h>

#include <iterator>
#include <algorithm>
#include <stdlib.h>
#include <thread>
#include <chrono>
#include <unordered_set>

using namespace syncd;
using namespace otaimeta;
using namespace std::placeholders;
using namespace swss;

int64_t time_zone_nanosecs = 0;

Syncd::Syncd(
    _In_ std::shared_ptr<otairedis::OtaiInterface> vendorOtai,
    _In_ std::shared_ptr<CommandLineOptions> cmd,
    _In_ bool needCheckLink) :
    m_commandLineOptions(cmd),
    m_linkCheckLoop(needCheckLink),
    m_vendorOtai(vendorOtai)
{
    SWSS_LOG_ENTER();
    //swss::Logger::getInstance().setMinPrio(swss::Logger::Priority(m_commandLineOptions->m_loglevel));//read level from db or set as default level
    SWSS_LOG_NOTICE("command line: %s", m_commandLineOptions->getCommandLineString().c_str());

    auto ccc = otairedis::ContextConfigContainer::loadFromFile(m_commandLineOptions->m_contextConfig.c_str());
    m_contextConfig = ccc->get(m_commandLineOptions->m_globalContext);
    if (m_contextConfig == nullptr)
    {
        SWSS_LOG_THROW("no context config defined at global context %u", m_commandLineOptions->m_globalContext);
    }
    m_manager = std::make_shared<FlexCounterManager>(m_vendorOtai, m_contextConfig->m_dbCounters);

    m_state_db = std::shared_ptr<DBConnector>(new DBConnector("STATE_DB", 0));
    m_linecardtable = std::unique_ptr<Table>(new Table(m_state_db.get(), "LINECARD"));
    m_curalarmtable = std::unique_ptr<Table>(new Table(m_state_db.get(), "CURALARM"));

    loadProfileMap();
    m_profileIter = m_profileMap.begin();

    // we need STATE_DB ASIC_DB and COUNTERS_DB
    m_dbAsic = std::make_shared<swss::DBConnector>(m_contextConfig->m_dbAsic, 0);
    m_dbFlexCounter = std::make_shared<swss::DBConnector>(m_contextConfig->m_dbFlex, 0);
    m_notifications = std::make_shared<RedisNotificationProducer>(m_contextConfig->m_dbAsic);
    m_selectableChannel = std::make_shared<RedisSelectableChannel>(
        m_dbAsic,
        ASIC_STATE_TABLE,
        REDIS_TABLE_GETRESPONSE,
        false);
    m_client = std::make_shared<RedisClient>(m_dbAsic, m_dbFlexCounter);

    m_processor = std::make_shared<NotificationProcessor>(m_mtxAlarmTable, m_notifications, m_contextConfig->m_dbAsic, m_client, std::bind(&Syncd::syncProcessNotification, this, _1), std::bind(&Syncd::handleLinecardStateChange, this, _1));
    m_handler = std::make_shared<NotificationHandler>(m_processor);
    m_ln.onLinecardStateChange = std::bind(&NotificationHandler::onLinecardStateChange, m_handler.get(), _1, _2);
    m_ln.onLinecardAlarm = std::bind(&NotificationHandler::onLinecardAlarm, m_handler.get(), _1, _2, _3);
    m_ln.onApsReportSwitchInfo = std::bind(&NotificationHandler::onApsReportSwitchInfo, m_handler.get(), _1, _2);
    m_ln.onOcmReportSpectrumPower = std::bind(&NotificationHandler::onOcmReportSpectrumPower, m_handler.get(), _1, _2, _3);
    m_ln.onOtdrReportResult = std::bind(&NotificationHandler::onOtdrReportResult, m_handler.get(), _1, _2, _3);
    m_handler->setLinecardNotifications(m_ln.getLinecardNotifications());
    m_restartQuery = std::make_shared<swss::NotificationConsumer>(m_dbAsic.get(), SYNCD_NOTIFICATION_CHANNEL_RESTARTQUERY);
    m_linecardStateNtf = std::make_shared<swss::NotificationConsumer>(m_dbAsic.get(), SYNCD_NOTIFICATION_CHANNEL_LINECARDSTATE);
    // TODO to be moved to ASIC_DB

    m_flexCounter = std::make_shared<swss::ConsumerTable>(m_dbFlexCounter.get(), FLEX_COUNTER_TABLE);
    m_flexCounterGroup = std::make_shared<swss::ConsumerTable>(m_dbFlexCounter.get(), FLEX_COUNTER_GROUP_TABLE);
    m_linecardConfigContainer = std::make_shared<otairedis::LinecardConfigContainer>();
    m_redisVidIndexGenerator = std::make_shared<otairedis::RedisVidIndexGenerator>(m_dbAsic, REDIS_KEY_VIDCOUNTER);
    m_virtualObjectIdManager =
        std::make_shared<otairedis::VirtualObjectIdManager>(
            m_commandLineOptions->m_globalContext,
            m_linecardConfigContainer,
            m_redisVidIndexGenerator);
    // TODO move to syncd object
    m_translator = std::make_shared<VirtualOidTranslator>(m_client, m_virtualObjectIdManager, vendorOtai);
    m_processor->m_translator = m_translator; // TODO as param
    m_smt.profileGetValue = std::bind(&Syncd::profileGetValue, this, _1, _2);
    m_smt.profileGetNextValue = std::bind(&Syncd::profileGetNextValue, this, _1, _2, _3);
    m_test_services = m_smt.getServiceMethodTable();
    otai_status_t status = vendorOtai->initialize(0, &m_test_services);
    if (status != OTAI_STATUS_SUCCESS)
    {
        SWSS_LOG_ERROR("FATAL: failed to otai_api_initialize: %s",
            otai_serialize_status(status).c_str());

        abort();
    }
    setOtaiApiLogLevel();

    m_linecardState = OTAI_OPER_STATUS_INACTIVE;
    SWSS_LOG_NOTICE("////////////////////////syncd started////////////////////////");
}

Syncd::~Syncd()
{
    SWSS_LOG_ENTER();

    // empty
}

void Syncd::processEvent(
    _In_ SelectableChannel& consumer)
{
    SWSS_LOG_ENTER();

    std::lock_guard<std::mutex> lock(m_mutex);

    do
    {
        swss::KeyOpFieldsValuesTuple kco;

        consumer.pop(kco);

        processSingleEvent(kco);
    }
    while (!consumer.empty());
}

otai_status_t Syncd::processSingleEvent(
    _In_ const swss::KeyOpFieldsValuesTuple& kco)
{
    SWSS_LOG_ENTER();

    auto& key = kfvKey(kco);
    auto& op = kfvOp(kco);

    SWSS_LOG_INFO("key: %s op: %s", key.c_str(), op.c_str());

    if (key.length() == 0)
    {
        SWSS_LOG_DEBUG("no elements in m_buffer");

        return OTAI_STATUS_SUCCESS;
    }

    if (op == REDIS_ASIC_STATE_COMMAND_CREATE)
        return processQuadEvent(OTAI_COMMON_API_CREATE, kco);

    if (op == REDIS_ASIC_STATE_COMMAND_REMOVE)
        return processQuadEvent(OTAI_COMMON_API_REMOVE, kco);

    if (op == REDIS_ASIC_STATE_COMMAND_SET)
        return processQuadEvent(OTAI_COMMON_API_SET, kco);

    if (op == REDIS_ASIC_STATE_COMMAND_GET)
        return processQuadEvent(OTAI_COMMON_API_GET, kco);

    SWSS_LOG_THROW("event op '%s' is not implemented, FIXME", op.c_str());
}

otai_status_t Syncd::processEntry(
    _In_ otai_object_meta_key_t metaKey,
    _In_ otai_common_api_t api,
    _In_ uint32_t attr_count,
    _In_ otai_attribute_t* attr_list)
{
    SWSS_LOG_ENTER();

    m_translator->translateVidToRid(metaKey);

    switch (api)
    {
    case OTAI_COMMON_API_CREATE:
        return m_vendorOtai->create(metaKey, OTAI_NULL_OBJECT_ID, attr_count, attr_list);

    case OTAI_COMMON_API_REMOVE:
        return m_vendorOtai->remove(metaKey);

    case OTAI_COMMON_API_SET:
        return m_vendorOtai->set(metaKey, attr_list);

    case OTAI_COMMON_API_GET:
        return m_vendorOtai->get(metaKey, attr_count, attr_list);

    default:

        SWSS_LOG_THROW("api %s not supported", otai_serialize_common_api(api).c_str());
    }
}

void Syncd::sendApiResponse(
    _In_ otai_common_api_t api,
    _In_ otai_status_t status,
    _In_ uint32_t object_count,
    _In_ otai_status_t* object_statuses)
{
    SWSS_LOG_ENTER();

    switch (api)
    {
    case OTAI_COMMON_API_CREATE:
    case OTAI_COMMON_API_REMOVE:
    case OTAI_COMMON_API_SET:
        break;

    default:
        SWSS_LOG_THROW("api %s not supported by this function",
            otai_serialize_common_api(api).c_str());
    }

    if (status != OTAI_STATUS_SUCCESS)
    {
        SWSS_LOG_ERROR("api %s failed in syncd mode: %s",
            otai_serialize_common_api(api).c_str(),
            otai_serialize_status(status).c_str());
    }

    std::vector<swss::FieldValueTuple> entry;

    for (uint32_t idx = 0; idx < object_count; idx++)
    {
        swss::FieldValueTuple fvt(otai_serialize_status(object_statuses[idx]), "");

        entry.push_back(fvt);
    }

    std::string strStatus = otai_serialize_status(status);

    SWSS_LOG_INFO("sending response for %s api with status: %s",
        otai_serialize_common_api(api).c_str(),
        strStatus.c_str());

    m_selectableChannel->set(strStatus, entry, REDIS_ASIC_STATE_COMMAND_GETRESPONSE);

    SWSS_LOG_INFO("response for %s api was send",
        otai_serialize_common_api(api).c_str());
}

void Syncd::processFlexCounterGroupEvent( // TODO must be moved to go via ASIC channel queue
    _In_ swss::ConsumerTable& consumer)
{
    SWSS_LOG_ENTER();

    std::lock_guard<std::mutex> lock(m_mutex);

    swss::KeyOpFieldsValuesTuple kco;

    consumer.pop(kco);

    auto& groupName = kfvKey(kco);
    auto& op = kfvOp(kco);
    auto& values = kfvFieldsValues(kco);

    SWSS_LOG_NOTICE("processFlexCounterGroupEvent group is %s", groupName.c_str());

    if (op == SET_COMMAND)
    {
        m_manager->addCounterPlugin(groupName, values);
    }
    else if (op == DEL_COMMAND)
    {
        m_manager->removeCounterPlugins(groupName);
    }
    else
    {
        SWSS_LOG_ERROR("unknown command: %s", op.c_str());
    }
}

void Syncd::processFlexCounterEvent( // TODO must be moved to go via ASIC channel queue
    _In_ swss::ConsumerTable& consumer)
{
    SWSS_LOG_ENTER();

    std::lock_guard<std::mutex> lock(m_mutex);

    swss::KeyOpFieldsValuesTuple kco;

    consumer.pop(kco);

    auto& key = kfvKey(kco);
    auto& op = kfvOp(kco);

    auto delimiter = key.find_first_of(":");

    if (delimiter == std::string::npos)
    {
        SWSS_LOG_ERROR("Failed to parse the key %s", key.c_str());

        return; // if key is invalid there is no need to process this event again
    }

    auto groupName = key.substr(0, delimiter);
    auto strVid = key.substr(delimiter + 1);

    otai_object_id_t vid = 0;
    otai_deserialize_object_id(strVid, vid);

    otai_object_id_t rid = 0;

    if (!m_translator->tryTranslateVidToRid(vid, rid))
    {
        SWSS_LOG_WARN("VID %s, was not found and will remove from counters now",
            otai_serialize_object_id(vid).c_str());

        op = DEL_COMMAND;
    }

    const auto values = kfvFieldsValues(kco);

    if (op == SET_COMMAND)
    {
        SWSS_LOG_NOTICE("m_manager addCounter vid is 0x%" PRIx64 " rid is 0x%" PRIx64 " group is %s", vid, rid, groupName.c_str());
        m_manager->addCounter(vid, rid, groupName, values);
    }
    else if (op == DEL_COMMAND)
    {
        SWSS_LOG_NOTICE("m_manager removeCounter vid is %s rid is 0x%" PRIx64 " group is %s", strVid.c_str(), rid, groupName.c_str());
        m_manager->removeCounter(vid, groupName);
    }
    else
    {
        SWSS_LOG_ERROR("unknown command: %s", op.c_str());
    }
}

void Syncd::syncUpdateRedisQuadEvent(
    _In_ otai_status_t status,
    _In_ otai_common_api_t api,
    _In_ const swss::KeyOpFieldsValuesTuple& kco)
{
    SWSS_LOG_ENTER();

    if (status != OTAI_STATUS_SUCCESS)
    {
        return;
    }

    // When in synchronous mode, we need to modify redis database when status
    // is success, since consumer table on synchronous mode is not making redis
    // changes and we only want to apply changes when api succeeded. This
    // applies to init view mode and apply view mode.

    const std::string& key = kfvKey(kco);

    auto& values = kfvFieldsValues(kco);

    otai_object_meta_key_t metaKey;
    otai_deserialize_object_meta_key(key, metaKey);

    switch (api)
    {
    case OTAI_COMMON_API_CREATE:
    {
        m_client->createAsicObject(metaKey, values);
        break;
    }
    case OTAI_COMMON_API_REMOVE:
    {
        m_client->removeAsicObject(metaKey);
        break;
    }
    case OTAI_COMMON_API_SET:
    {
        auto& first = values.at(0);

        auto& attr = fvField(first);
        auto& value = fvValue(first);

        auto m = otai_metadata_get_attr_metadata_by_attr_id_name(attr.c_str());

        if (m == NULL)
        {
            SWSS_LOG_THROW("invalid attr id: %s", attr.c_str());
        }

        if (m->isrecoverable == false)
        {
            break;
        }

        m_client->setAsicObject(metaKey, attr, value);
        break;
    }

    case OTAI_COMMON_API_GET:
        break; // ignore get since get is not modifying db

    default:

        SWSS_LOG_THROW("api %d is not supported", api);
    }
}

otai_status_t Syncd::processQuadEvent(
    _In_ otai_common_api_t api,
    _In_ const swss::KeyOpFieldsValuesTuple& kco)
{
    SWSS_LOG_ENTER();

    const std::string& key = kfvKey(kco);
    const std::string& op = kfvOp(kco);

    const std::string& strObjectId = key.substr(key.find(":") + 1);

    otai_object_meta_key_t metaKey;
    otai_deserialize_object_meta_key(key, metaKey);

    if (!otai_metadata_is_object_type_valid(metaKey.objecttype))
    {
        SWSS_LOG_THROW("invalid object type %s", key.c_str());
    }

    auto& values = kfvFieldsValues(kco);

    for (auto& v : values)
    {
        SWSS_LOG_DEBUG("attr: %s: %s", fvField(v).c_str(), fvValue(v).c_str());
    }

    OtaiAttributeList list(metaKey.objecttype, values, false);

    /*
     * Attribute list can't be const since we will use it to translate VID to
     * RID in place.
     */

    otai_attribute_t* attr_list = list.get_attr_list();
    uint32_t attr_count = list.get_attr_count();

    /*
     * NOTE: This check pointers must be executed before init view mode, since
     * this methods replaces pointers from orchagent memory space to syncd
     * memory space.
     */

    if (api == OTAI_COMMON_API_CREATE || api == OTAI_COMMON_API_SET)
    {
        /*
         * We don't need to clear those pointers on linecard remove (even last),
         * since those pointers will reside inside attributes, also otairedis
         * will internally check whether pointer is null or not, so we here
         * will receive all notifications, but redis only those that were set.
         *
         * TODO: must be done per linecard, and linecard may not exists yet
         */

        m_handler->updateNotificationsPointers(metaKey.objecttype, attr_count, attr_list);
    }

    if (api != OTAI_COMMON_API_GET)
    {
        /*
         * NOTE: we can also call translate on get, if otairedis will clean
         * buffer so then all OIDs will be NULL, and translation will also
         * convert them to NULL.
         */

        SWSS_LOG_DEBUG("translating VID to RIDs on all attributes");

        m_translator->translateVidToRid(metaKey.objecttype, attr_count, attr_list);
    }

    auto info = otai_metadata_get_object_type_info(metaKey.objecttype);

    otai_status_t status;

    if (info->isnonobjectid)
    {
        status = processEntry(metaKey, api, attr_count, attr_list);
    }
    else
    {
        status = processOid(metaKey.objecttype, strObjectId, api, attr_count, attr_list);
    }

    if (api == OTAI_COMMON_API_GET)
    {
        if (status != OTAI_STATUS_SUCCESS)
        {
            SWSS_LOG_INFO("get API for key: %s op: %s returned status: %s",
                key.c_str(),
                op.c_str(),
                otai_serialize_status(status).c_str());
        }

        // extract linecard VID from any object type

        otai_object_id_t linecardVid = VidManager::linecardIdQuery(metaKey.objectkey.key.object_id);

        sendGetResponse(metaKey.objecttype, strObjectId, linecardVid, status, attr_count, attr_list);
    }
    else if (status != OTAI_STATUS_SUCCESS)
    {
        sendApiResponse(api, status);

        if (info->isobjectid && api == OTAI_COMMON_API_SET)
        {
            otai_object_id_t vid = metaKey.objectkey.key.object_id;
            otai_object_id_t rid = m_translator->translateVidToRid(vid);

            SWSS_LOG_ERROR("VID: %s RID: %s",
                otai_serialize_object_id(vid).c_str(),
                otai_serialize_object_id(rid).c_str());
        }

        for (const auto& v : values)
        {
            SWSS_LOG_ERROR("attr: %s: %s", fvField(v).c_str(), fvValue(v).c_str());
        }
    }
    else // non GET api, status is SUCCESS
    {
        sendApiResponse(api, status);
    }

    syncUpdateRedisQuadEvent(status, api, kco);

    return status;
}

otai_status_t Syncd::processOid(
    _In_ otai_object_type_t objectType,
    _In_ const std::string& strObjectId,
    _In_ otai_common_api_t api,
    _In_ uint32_t attr_count,
    _In_ otai_attribute_t* attr_list)
{
    SWSS_LOG_ENTER();

    otai_object_id_t object_id;
    otai_deserialize_object_id(strObjectId, object_id);

    SWSS_LOG_DEBUG("calling %s for %s",
        otai_serialize_common_api(api).c_str(),
        otai_serialize_object_type(objectType).c_str());

    /*
     * We need to do translate vid/rid except for create, since create will
     * create new RID value, and we will have to map them to VID we received in
     * create query.
     */

    auto info = otai_metadata_get_object_type_info(objectType);

    if (info->isnonobjectid)
    {
        SWSS_LOG_THROW("passing non object id %s as generic object", info->objecttypename);
    }

    switch (api)
    {
    case OTAI_COMMON_API_CREATE:
        return processOidCreate(objectType, strObjectId, attr_count, attr_list);

    case OTAI_COMMON_API_REMOVE:
        return processOidRemove(objectType, strObjectId);

    case OTAI_COMMON_API_SET:
        return processOidSet(objectType, strObjectId, attr_list);

    case OTAI_COMMON_API_GET:
        return processOidGet(objectType, strObjectId, attr_count, attr_list);

    default:

        SWSS_LOG_THROW("common api (%s) is not implemented", otai_serialize_common_api(api).c_str());
    }
}

otai_status_t Syncd::processOidCreate(
    _In_ otai_object_type_t objectType,
    _In_ const std::string& strObjectId,
    _In_ uint32_t attr_count,
    _In_ otai_attribute_t* attr_list)
{
    SWSS_LOG_ENTER();

    otai_object_id_t objectVid;
    otai_deserialize_object_id(strObjectId, objectVid);

    // Object id is VID, we can use it to extract linecard id.

    otai_object_id_t linecardVid = VidManager::linecardIdQuery(objectVid);

    otai_object_id_t linecardRid = OTAI_NULL_OBJECT_ID;

    if (objectType == OTAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_NOTICE("creating linecard number %zu", m_linecards.size() + 1);
    }
    else
    {
        /*
         * When we are creating linecard, then linecardId parameter is ignored, but
         * we can't convert it using vid to rid map, since rid doesn't exist
         * yet, so skip translate for linecard, but use translate for all other
         * objects.
         */

        linecardRid = m_translator->translateVidToRid(linecardVid);
    }

    preprocessOidOps(objectType, attr_list, attr_count);

    otai_object_id_t objectRid;

    otai_status_t status = m_vendorOtai->create(objectType, &objectRid, linecardRid, attr_count, attr_list);

    if (status == OTAI_STATUS_SUCCESS)
    {

        otai_object_id_t objectVidOld;

        if (m_translator->tryTranslateRidToVid(objectRid, objectVidOld))
        {
            m_translator->eraseRidAndVid(objectRid, objectVidOld);

            m_client->removeAsicObject(objectVidOld);
        }

        /*
         * Object was created so new object id was generated we need to save
         * virtual id's to redis db.
         */

        m_translator->insertRidAndVid(objectRid, objectVid);

        SWSS_LOG_INFO("saved VID %s to RID %s",
            otai_serialize_object_id(objectVid).c_str(),
            otai_serialize_object_id(objectRid).c_str());

        if (objectType == OTAI_OBJECT_TYPE_LINECARD)
        {
            /*
             * All needed data to populate linecard should be obtained inside OtaiLinecard
             * constructor, like getting all queues, ports, etc.
             */

            m_linecards[linecardVid] = std::make_shared<OtaiLinecard>(linecardVid, objectRid, m_client, m_translator, m_vendorOtai);
        }
    }

    return status;
}

otai_status_t Syncd::processOidRemove(
    _In_ otai_object_type_t objectType,
    _In_ const std::string& strObjectId)
{
    SWSS_LOG_ENTER();

    otai_object_id_t objectVid;
    otai_deserialize_object_id(strObjectId, objectVid);

    otai_object_id_t rid = m_translator->translateVidToRid(objectVid);

    otai_status_t status = m_vendorOtai->remove(objectType, rid);

    if (status == OTAI_STATUS_SUCCESS)
    {
        // remove all related objects from REDIS DB and also from existing
        // object references since at this point they are no longer valid

        m_translator->eraseRidAndVid(rid, objectVid);

        if (objectType == OTAI_OBJECT_TYPE_LINECARD)
        {
            /*
             * On remove linecard there should be extra action all local objects
             * and redis object should be removed on remove linecard local and
             * redis db objects should be cleared.
             *
             * Currently we don't want to remove linecard so we don't need this
             * method, but lets put this as a safety check.
             */

            SWSS_LOG_THROW("remove linecard is not implemented, FIXME");
        }
    }

    return status;
}

otai_status_t Syncd::processOidSet(
    _In_ otai_object_type_t objectType,
    _In_ const std::string& strObjectId,
    _In_ otai_attribute_t* attr)
{
    SWSS_LOG_ENTER();

    otai_object_id_t objectVid;
    otai_deserialize_object_id(strObjectId, objectVid);

    otai_object_id_t rid = m_translator->translateVidToRid(objectVid);

    preprocessOidOps(objectType, attr, 1);

    otai_status_t status = m_vendorOtai->set(objectType, rid, attr);

    return status;
}

otai_status_t Syncd::processOidGet(
    _In_ otai_object_type_t objectType,
    _In_ const std::string& strObjectId,
    _In_ uint32_t attr_count,
    _In_ otai_attribute_t* attr_list)
{
    SWSS_LOG_ENTER();

    otai_object_id_t objectVid;
    otai_deserialize_object_id(strObjectId, objectVid);

    otai_object_id_t rid = m_translator->translateVidToRid(objectVid);

    return m_vendorOtai->get(objectType, rid, attr_count, attr_list);
}

const char* Syncd::profileGetValue(
    _In_ otai_linecard_profile_id_t profile_id,
    _In_ const char* variable)
{
    SWSS_LOG_ENTER();

    if (variable == NULL)
    {
        SWSS_LOG_WARN("variable is null");
        return NULL;
    }

    auto it = m_profileMap.find(variable);

    if (it == m_profileMap.end())
    {
        SWSS_LOG_NOTICE("%s: NULL", variable);
        return NULL;
    }

    SWSS_LOG_NOTICE("%s: %s", variable, it->second.c_str());

    return it->second.c_str();
}

int Syncd::profileGetNextValue(
    _In_ otai_linecard_profile_id_t profile_id,
    _Out_ const char** variable,
    _Out_ const char** value)
{
    SWSS_LOG_ENTER();

    if (value == NULL)
    {
        SWSS_LOG_INFO("resetting profile map iterator");

        m_profileIter = m_profileMap.begin();
        return 0;
    }

    if (variable == NULL)
    {
        SWSS_LOG_WARN("variable is null");
        return -1;
    }

    if (m_profileIter == m_profileMap.end())
    {
        SWSS_LOG_INFO("iterator reached end");
        return -1;
    }

    *variable = m_profileIter->first.c_str();
    *value = m_profileIter->second.c_str();

    SWSS_LOG_INFO("key: %s:%s", *variable, *value);

    m_profileIter++;

    return 0;
}

void Syncd::loadProfileMap()
{
    SWSS_LOG_ENTER();

    if (m_commandLineOptions->m_profileMapFile.size() == 0)
    {
        SWSS_LOG_NOTICE("profile map file not specified");
        return;
    }

    std::ifstream profile(m_commandLineOptions->m_profileMapFile);

    if (!profile.is_open())
    {
        SWSS_LOG_ERROR("failed to open profile map file: %s: %s",
            m_commandLineOptions->m_profileMapFile.c_str(),
            strerror(errno));

        exit(EXIT_FAILURE);
    }


    std::string line;
    while (getline(profile, line))
    {
        if (line.size() > 0 && (line[0] == '#' || line[0] == ';'))
        {
            continue;
        }

        size_t pos = line.find("=");

        if (pos == std::string::npos)
        {
            SWSS_LOG_WARN("not found '=' in line %s", line.c_str());
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        m_profileMap[key] = value;

        SWSS_LOG_INFO("insert: %s:%s", key.c_str(), value.c_str());
    }
}

void Syncd::sendGetResponse(
    _In_ otai_object_type_t objectType,
    _In_ const std::string& strObjectId,
    _In_ otai_object_id_t linecardVid,
    _In_ otai_status_t status,
    _In_ uint32_t attr_count,
    _In_ otai_attribute_t* attr_list)
{
    SWSS_LOG_ENTER();

    std::vector<swss::FieldValueTuple> entry;

    if (status == OTAI_STATUS_SUCCESS)
    {
        m_translator->translateRidToVid(objectType, linecardVid, attr_count, attr_list);

        /*
         * Normal serialization + translate RID to VID.
         */

        entry = OtaiAttributeList::serialize_attr_list(
            objectType,
            attr_count,
            attr_list,
            false);
    }
    else
    {
        /*
         * Some other error, don't send attributes at all.
         */
    }

    for (const auto& e : entry)
    {
        SWSS_LOG_DEBUG("attr: %s: %s", fvField(e).c_str(), fvValue(e).c_str());
    }

    std::string strStatus = otai_serialize_status(status);

    SWSS_LOG_INFO("sending response for GET api with status: %s", strStatus.c_str());

    /*
     * Since we have only one get at a time, we don't have to serialize object
     * type and object id, only get status is required to be returned.  Get
     * response will not put any data to table, only queue is used.
     */

    m_selectableChannel->set(strStatus, entry, REDIS_ASIC_STATE_COMMAND_GETRESPONSE);

    SWSS_LOG_INFO("response for GET api was send");
}

// TODO for future we can have each linecard in separate redis db index or even
// some linecards in the same db index and some in separate.  Current redis get
// asic view is assuming all linecards are in the same db index an also some
// operations per linecard are accessing data base in OtaiLinecard class.  This
// needs to be reorganised to access database per linecard basis and get only
// data that corresponds to each particular linecard and access correct db index.

void Syncd::onSyncdStart()
{
    SWSS_LOG_ENTER();

    std::lock_guard<std::mutex> lock(m_mutex);

    /*
     * It may happen that after initialize we will receive some port
     * notifications with port'ids that are not in redis db yet, so after
     * checking VIDTORID map there will be entries and translate_vid_to_rid
     * will generate new id's for ports, this may cause race condition so we
     * need to use a lock here to prevent that.
     */

    SWSS_LOG_TIMER("on syncd start");

    SWSS_LOG_NOTICE("performing hard reinit since COLD start was performed");

    /*
     * Linecard was restarted in hard way, we need to perform hard reinit and
     * recreate linecards map.
     */

    if (m_linecards.size())
    {
        SWSS_LOG_THROW("performing hard reinit, but there are %zu linecards defined, bug!", m_linecards.size());
    }

    HardReiniter hr(m_client, m_translator, m_vendorOtai, m_handler, m_manager);

    m_linecards = hr.hardReinit();

    SWSS_LOG_NOTICE("hard reinit succeeded");
}

void Syncd::sendShutdownRequestAfterException()
{
    SWSS_LOG_ENTER();

    std::lock_guard<std::mutex> lock(m_mutex);

    try
    {
        if (m_linecards.size())
        {
            for (auto& kvp : m_linecards)
            {
                auto msg = otai_serialize_linecard_oper_status(kvp.first, OTAI_OPER_STATUS_INACTIVE);
                m_processor->sendNotification(OTAI_LINECARD_NOTIFICATION_NAME_LINECARD_STATE_CHANGE, msg);
            }
        }

        SWSS_LOG_NOTICE("notification send successfully");
    }
    catch (const std::exception& e)
    {
        SWSS_LOG_ERROR("Runtime error: %s", e.what());
    }
    catch (...)
    {
        SWSS_LOG_ERROR("Unknown runtime error");
    }
}

void Syncd::otaiLoglevelNotify(
    _In_ std::string strApi,
    _In_ std::string strLogLevel)
{
    SWSS_LOG_ENTER();

    try
    {
        otai_log_level_t logLevel;
        otai_deserialize_log_level(strLogLevel, logLevel);

        otai_api_t api;
        otai_deserialize_api(strApi, api);

        otai_status_t status = m_vendorOtai->logSet(api, logLevel);

        if (status == OTAI_STATUS_SUCCESS)
        {
            SWSS_LOG_NOTICE("Setting OTAI loglevel %s on %s", strLogLevel.c_str(), strApi.c_str());
        }
        else
        {
            SWSS_LOG_INFO("set loglevel failed: %s", otai_serialize_status(status).c_str());
        }
    }
    catch (const std::exception& e)
    {
        SWSS_LOG_ERROR("Failed to set loglevel to %s on %s: %s",
            strLogLevel.c_str(),
            strApi.c_str(),
            e.what());
    }
}

void Syncd::setOtaiApiLogLevel()
{
    SWSS_LOG_ENTER();

    // We start from 1 since 0 is OTAI_API_UNSPECIFIED.

    for (uint32_t idx = 1; idx < otai_metadata_enum_otai_api_t.valuescount; ++idx)
    {
        // NOTE: link to db is singleton, so if we would want multiple Syncd
        // instances running at the same process, we need to have logger
        // registrar similar to net link messages

        swss::Logger::linkToDb(
            otai_metadata_enum_otai_api_t.valuesnames[idx],
            std::bind(&Syncd::otaiLoglevelNotify, this, _1, _2),
            otai_serialize_log_level(OTAI_LOG_LEVEL_WARN));
    }

    swss::Logger::linkToDb(
        otai_metadata_enum_otai_api_t.valuesnames[OTAI_API_LLDP],
        std::bind(&Syncd::otaiLoglevelNotify, this, _1, _2),
        otai_serialize_log_level(OTAI_LOG_LEVEL_ERROR));
}

otai_status_t Syncd::removeAllLinecards()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("Removing all linecards");

    // TODO mutex ?

    otai_status_t result = OTAI_STATUS_SUCCESS;

    for (auto& sw : m_linecards)
    {
        auto rid = sw.second->getRid();

        auto strRid = otai_serialize_object_id(rid);

        SWSS_LOG_TIMER("removing linecard RID %s", strRid.c_str());

        auto status = m_vendorOtai->remove(OTAI_OBJECT_TYPE_LINECARD, rid);

        if (status != OTAI_STATUS_SUCCESS)
        {
            SWSS_LOG_NOTICE("Can't delete a linecard RID %s: %s",
                strRid.c_str(),
                otai_serialize_status(status).c_str());

            result = status;
        }
    }

    return result;
}

void Syncd::syncProcessNotification(
    _In_ const swss::KeyOpFieldsValuesTuple& item)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    SWSS_LOG_ENTER();

    m_processor->syncProcessNotification(item);
}

int64_t get_time_zone_offset_milliseconds()
{
    time_t t1 = 0;
    time_t t2 = 0;
    struct tm tm_local;
    struct tm tm_utc;
    memset(&tm_local, 0, sizeof(struct tm));
    memset(&tm_utc, 0, sizeof(struct tm));

    time(&t1);
    t2 = t1;
    localtime_r(&t1, &tm_local);
    t1 = mktime(&tm_local);
    gmtime_r(&t2, &tm_utc);
    t2 = mktime(&tm_utc);
    return (t1 - t2) * 1000;
}

void Syncd::run()
{
    SWSS_LOG_ENTER();

    //change from local 00:00:00 to UTC 00:00:00
    //time_zone_nanosecs = get_time_zone_offset_milliseconds();

    volatile bool runMainLoop = true;

    std::shared_ptr<swss::Select> s = std::make_shared<swss::Select>();

    while (m_linkCheckLoop)
    {
        otai_status_t status;
        bool isLinkUp = false;
        status = m_vendorOtai->linkCheck(&isLinkUp);
        if (status == OTAI_STATUS_SUCCESS && isLinkUp == true)
        {
            SWSS_LOG_NOTICE("Link is up");
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    m_linecardState = OTAI_OPER_STATUS_ACTIVE;

    try
    {
        onSyncdStart();

        // create notifications processing thread after we create_linecard to
        // make sure, we have linecard_id translated to VID before we start
        // processing possible quick fdb notifications, and pointer for
        // notification queue is created before we create linecard
        m_processor->startNotificationsProcessingThread();

        SWSS_LOG_NOTICE("syncd listening for events");

        s->addSelectable(m_selectableChannel.get());
        s->addSelectable(m_restartQuery.get());
        s->addSelectable(m_linecardStateNtf.get());
        s->addSelectable(m_flexCounter.get());
        s->addSelectable(m_flexCounterGroup.get());

        SWSS_LOG_NOTICE("starting main loop");
    }
    catch (const std::exception& e)
    {
        SWSS_LOG_ERROR("Runtime error during syncd init: %s", e.what());

        sendShutdownRequestAfterException();

        s = std::make_shared<swss::Select>();

        s->addSelectable(m_restartQuery.get());

        SWSS_LOG_NOTICE("starting main loop, ONLY restart query");

        runMainLoop = false;

        exit(EXIT_FAILURE);
    }

    std::vector<std::string> linecardkey;
    while (0 == linecardkey.size())
    {
        m_linecardtable->getKeys(linecardkey);
        SWSS_LOG_NOTICE("Waiting for Linecard...");
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
    for (const auto& value : linecardkey)
    {
        m_linecardtable->hset(value, "oper-status", "ACTIVE");
        m_linecardtable->hset(value, "slot-status", "Ready");
    }

    m_linecardtable->flush();

    for (auto& linecard : m_linecards)
    {
        auto msg = otai_serialize_linecard_oper_status(linecard.first, OTAI_OPER_STATUS_ACTIVE);
        SWSS_LOG_NOTICE("linecard is active, send this message to swss");
        m_processor->sendNotification(OTAI_LINECARD_NOTIFICATION_NAME_LINECARD_STATE_CHANGE, msg);
    }

    while (runMainLoop)
    {
        try
        {
            swss::Selectable* sel = NULL;

            int result = s->select(&sel);

            if (sel == m_linecardStateNtf.get())
            {
                SWSS_LOG_NOTICE("linecard state change");

                otai_oper_status_t linecard_state = handleLinecardState(*m_linecardStateNtf);

                if (m_linecardState != linecard_state)
                {
                    if (linecard_state == OTAI_OPER_STATUS_INACTIVE)
                    {
                        m_manager->removeAllCounters();
                        while (!m_selectableChannel->empty())
                        {
                            swss::KeyOpFieldsValuesTuple kco;
                            m_selectableChannel->pop(kco);
                        }
                    }
                    else if (linecard_state == OTAI_OPER_STATUS_ACTIVE)
                    {
                        //clear current alarm table.
                        otai_attribute_t attr;
                        attr.id = OTAI_LINECARD_ATTR_COLLECT_LINECARD_ALARM;
                        attr.value.booldata = true;
                        preprocessOidOps(OTAI_OBJECT_TYPE_LINECARD, &attr, 1);

                        SoftReiniter sr(m_client, m_translator, m_vendorOtai, m_manager);
                        sr.softReinit();
                    }
                    m_linecardState = linecard_state;
                    auto strOperStatus = otai_serialize_enum(m_linecardState, &otai_metadata_enum_otai_oper_status_t, true);
                    m_linecardtable->getKeys(linecardkey);
                    for (const auto& value : linecardkey)
                    {
                        m_linecardtable->hset(value, "oper-status", strOperStatus.c_str());
                    }
                }
            }
            else if (sel == m_restartQuery.get())
            {
                /*
                 * This is actual a bad design, since selectable may pick up
                 * multiple events from the queue, and after restart those
                 * events will be forgotten since they were consumed already and
                 * this may lead to forget populate object table which will
                 * lead to unable to find some objects.
                 */

                SWSS_LOG_NOTICE("is asic queue empty: %d", m_selectableChannel->empty());

                while (!m_selectableChannel->empty())
                {
                    processEvent(*m_selectableChannel.get());
                }

                SWSS_LOG_NOTICE("drained queue");

                // break out the event handling loop to shutdown syncd
                runMainLoop = false;

                break;
            }
            else if (sel == m_flexCounter.get())
            {
                processFlexCounterEvent(*(swss::ConsumerTable*)sel);
            }
            else if (sel == m_flexCounterGroup.get())
            {
                processFlexCounterGroupEvent(*(swss::ConsumerTable*)sel);
            }
            else if (sel == m_selectableChannel.get())
            {
                processEvent(*m_selectableChannel.get());
            }
            else
            {
                SWSS_LOG_ERROR("select failed: %d", result);
                exit(EXIT_FAILURE);
            }
        }
        catch (const std::runtime_error& re)
        {
            SWSS_LOG_ERROR("Runtime error: %s", re.what());

            if (NULL == strstr(re.what(), "failed to create object"))
            {
                sendShutdownRequestAfterException();

                s = std::make_shared<swss::Select>();

                s->addSelectable(m_restartQuery.get());

                runMainLoop = false;
            }

            exit(EXIT_FAILURE);
        }
        catch (const std::exception& e)
        {
            SWSS_LOG_ERROR("Runtime error: %s", e.what());

            sendShutdownRequestAfterException();

            s = std::make_shared<swss::Select>();

            s->addSelectable(m_restartQuery.get());

            runMainLoop = false;

            exit(EXIT_FAILURE);
        }
    }

    m_manager->removeAllCounters();

    for (auto& linecard : m_linecards)
    {
        auto msg = otai_serialize_linecard_oper_status(linecard.first, OTAI_OPER_STATUS_INACTIVE);
        SWSS_LOG_NOTICE("syncd will exit, send this message to swss");
        m_processor->sendNotification(OTAI_LINECARD_NOTIFICATION_NAME_LINECARD_STATE_CHANGE, msg);
    }

    otai_status_t status = removeAllLinecards();

    // Stop notification thread after removing linecard
    m_processor->stopNotificationsProcessingThread();

    SWSS_LOG_NOTICE("calling api uninitialize");

    status = m_vendorOtai->uninitialize();

    if (status != OTAI_STATUS_SUCCESS)
    {
        SWSS_LOG_ERROR("failed to uninitialize api: %s", otai_serialize_status(status).c_str());
    }

    SWSS_LOG_NOTICE("uninitialize finished");
}

syncd_restart_type_t Syncd::handleRestartQuery(
    _In_ swss::NotificationConsumer& restartQuery)
{
    SWSS_LOG_ENTER();

    std::string op;
    std::string data;
    std::vector<swss::FieldValueTuple> values;

    restartQuery.pop(op, data, values);

    SWSS_LOG_NOTICE("received %s linecard shutdown event", op.c_str());

    return RequestShutdownCommandLineOptions::stringToRestartType(op);
}

otai_oper_status_t Syncd::handleLinecardState(
    _In_ swss::NotificationConsumer& linecardState)
{
    SWSS_LOG_ENTER();

    std::string op;
    std::string data;
    std::vector<swss::FieldValueTuple> values;

    linecardState.pop(op, data, values);

    SWSS_LOG_NOTICE("received %s linecard state event", op.c_str());

    otai_oper_status_t linecard_oper_status;
    otai_object_id_t linecard_id;

    otai_deserialize_linecard_oper_status(data, linecard_id, linecard_oper_status);

    return linecard_oper_status;
}

void Syncd::handleLinecardStateChange(
    _In_ const otai_oper_status_t& oper_status)
{
    SWSS_LOG_ENTER();

    if (oper_status == OTAI_OPER_STATUS_INACTIVE)
    {
        m_manager->removeAllCounters();
    }

    return;
}

void Syncd::preprocessOidOps(otai_object_type_t objectType, otai_attribute_t* attr_list, uint32_t attr_count)
{
    SWSS_LOG_ENTER();

    if (OTAI_OBJECT_TYPE_LINECARD == objectType)
    {
        for (uint32_t idx = 0; idx < attr_count; idx++)
        {
            if (OTAI_LINECARD_ATTR_COLLECT_LINECARD_ALARM == attr_list[idx].id)
            {
                // clear current alarm table
                std::lock_guard<std::mutex> lock_alarm(m_mtxAlarmTable);
                int nCount = 0;
                vector<string> almkeys;
                m_curalarmtable->getKeys(almkeys);
                for (auto& key : almkeys)
                {
                    if (string::npos == key.find("SLOT_COMM_FAIL"))//skip alarms which are not generated by linecard.
                    {
                        m_curalarmtable->del(key);
                        nCount++;
                    }
                }
                SWSS_LOG_NOTICE("%d items in CURALARM table are cleared.", nCount);

                break;
            }
        }
    }
}
