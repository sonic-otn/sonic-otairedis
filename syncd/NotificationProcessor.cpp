#include "NotificationProcessor.h"
#include "RedisClient.h"

#include "meta/otai_serialize.h"
#include "meta/OtaiAttributeList.h"

#include "swss/logger.h"
#include "swss/notificationproducer.h"

#include "swss/json.hpp"
#include <inttypes.h>
#include "Common.h"

using json = nlohmann::json;
using namespace syncd;
using namespace otaimeta;
using namespace swss;

NotificationProcessor::NotificationProcessor(
    _In_ std::mutex& mtxAlarm,
    _In_ std::shared_ptr<NotificationProducerBase> producer,
    _In_ std::string dbAsic,
    _In_ std::shared_ptr<RedisClient> client,
    _In_ std::function<void(const swss::KeyOpFieldsValuesTuple&)> synchronizer,
    _In_ std::function<void(const otai_oper_status_t&)> linecard_state_change_handler) :
    m_mtxAlarmTable(mtxAlarm),
    m_synchronizer(synchronizer),
    m_linecard_state_change_handler(linecard_state_change_handler),
    m_client(client),
    m_notifications(producer),
    m_dbAsic(dbAsic)
{
    SWSS_LOG_ENTER();

    m_runThread = false;

    m_notificationQueue = std::make_shared<NotificationQueue>();
    m_state_db = std::shared_ptr<DBConnector>(new DBConnector("STATE_DB", 0));
    m_stateAlarmable = std::unique_ptr<Table>(new Table(m_state_db.get(), "CURALARM"));
    m_stateOLPSwitchInfoTbl = std::unique_ptr<Table>(new Table(m_state_db.get(), "OLP_SWITCH_INFO"));
    m_stateOcmTable = std::unique_ptr<Table>(new Table(m_state_db.get(), STATE_OT_OCM_TABLE_NAME));

    m_stateOtdrTable = std::make_shared<Table>(m_state_db.get(), STATE_OT_OTDR_TABLE_NAME);
    m_stateOtdrEventTable = std::make_shared<Table>(m_state_db.get(), "OTDR_EVENT");

    m_history_db = std::shared_ptr<DBConnector>(new DBConnector("HISTORY_DB", 0));
    m_historyAlarmTable = std::unique_ptr<Table>(new Table(m_history_db.get(), "HISALARM"));
    m_historyEventTable = std::unique_ptr<Table>(new Table(m_history_db.get(), "HISEVENT"));

    m_historyOtdrTable = std::make_shared<Table>(m_history_db.get(), "OTDR");
    m_historyOtdrEventTable = std::make_shared<Table>(m_history_db.get(), "OTDR_EVENT");

    initOtdrScanTimeSet();

    m_ttlPM15Min = EXIPRE_TIME_SECONDS_2DAYS;
    m_ttlPM24Hour = EXIPRE_TIME_SECONDS_7DAYS;
    m_ttlAlarm = EXIPRE_TIME_SECONDS_7DAYS;
}

NotificationProcessor::~NotificationProcessor()
{
    SWSS_LOG_ENTER();

    stopNotificationsProcessingThread();
}

void NotificationProcessor::initOtdrScanTimeSet()
{
    SWSS_LOG_ENTER();

    std::map<std::string, std::set<uint64_t>> scanTimeSet;

    std::string pattern = "OTDR:*";

    auto keys = m_history_db->keys(pattern);

    for (auto &k : keys)
    {
        auto name = m_history_db->hget(k, "name");
        auto strScanTime = m_history_db->hget(k, "scan-time");

        if (name == nullptr || strScanTime == nullptr)
        {
            continue;
        }

        uint64_t scanTime = 0;
        otai_deserialize_number(*strScanTime, scanTime);

        scanTimeSet[*name].insert(scanTime);
    }

    for (auto &s : scanTimeSet)
    {
        for (auto &t : s.second)
        {
            m_otdrScanTimeQueue[s.first].push(t);
        }
    }
}

void NotificationProcessor::sendNotification(
    _In_ const std::string& op,
    _In_ const std::string& data,
    _In_ std::vector<swss::FieldValueTuple> entry)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("%s %s", op.c_str(), data.c_str());

    m_notifications->send(op, data, entry);

    SWSS_LOG_DEBUG("notification send successfully");
}

void NotificationProcessor::sendNotification(
    _In_ const std::string& op,
    _In_ const std::string& data)
{
    SWSS_LOG_ENTER();

    std::vector<swss::FieldValueTuple> entry;

    sendNotification(op, data, entry);
}

void NotificationProcessor::process_on_linecard_state_change(
    _In_ otai_object_id_t linecard_rid,
    _In_ otai_oper_status_t linecard_oper_status)
{
    SWSS_LOG_ENTER();

    otai_object_id_t linecard_vid = m_translator->translateRidToVid(linecard_rid, OTAI_NULL_OBJECT_ID);

    auto s = otai_serialize_linecard_oper_status(linecard_vid, linecard_oper_status);

    SWSS_LOG_NOTICE("linecard state change to %s", s.c_str());

    sendNotification(OTAI_LINECARD_NOTIFICATION_NAME_LINECARD_STATE_CHANGE, s);

    swss::DBConnector db(m_dbAsic, 0);
    swss::NotificationProducer linecard_state(&db, SYNCD_NOTIFICATION_CHANNEL_LINECARDSTATE);

    std::vector<swss::FieldValueTuple> values;
    linecard_state.send(s, s, values);
}

void NotificationProcessor::handle_linecard_state_change(
    _In_ const std::string& data)
{
    SWSS_LOG_ENTER();

    otai_oper_status_t linecard_oper_status;
    otai_object_id_t linecard_id;

    otai_deserialize_linecard_oper_status(data, linecard_id, linecard_oper_status);

    process_on_linecard_state_change(linecard_id, linecard_oper_status);
}

void NotificationProcessor::handle_olp_switch_notify(
    _In_ const std::string& data,
    _In_ const std::vector<FieldValueTuple>& fv)
{
    SWSS_LOG_ENTER();

    json j = json::parse(data);

    otai_object_id_t vid;
    otai_object_id_t rid;

    otai_deserialize_object_id(j["rid"], rid);

    if (!m_translator->tryTranslateRidToVid(rid, vid))
    {
        SWSS_LOG_ERROR("translate rid to vid failed, rid=0x%" PRIx64, rid);
        return;
    }
    auto counters_db = std::shared_ptr<swss::DBConnector>(new swss::DBConnector("COUNTERS_DB", 0));
    std::string strVid = otai_serialize_object_id(vid);
    auto key = counters_db->hget(COUNTERS_OT_APS_NAME_MAP, strVid);
    if (key == nullptr)
    {
        SWSS_LOG_ERROR("cannot get name map, %s %s", COUNTERS_OT_APS_NAME_MAP, strVid.c_str());
        return;
    }
    std::string strKey = *key;
    strKey += "_";
    strKey += j["time-stamp"];

    SWSS_LOG_NOTICE("record olp switch info, key=%s", strKey.c_str());

    m_stateOLPSwitchInfoTbl->set(strKey, fv);
}

void NotificationProcessor::handle_ocm_spectrum_power_notify(
    _In_ const std::string& data,
    _In_ const std::vector<FieldValueTuple>& fv)
{
    SWSS_LOG_ENTER();

    json j = json::parse(data);

    otai_object_id_t vid;
    otai_object_id_t rid;
    otai_spectrum_power_list_t list;

    otai_object_id_t linecard_vid;
    otai_object_id_t linecard_rid;

    otai_deserialize_object_id(j["ocm_id"], rid);
    otai_deserialize_object_id(j["linecard_rid"], linecard_rid);
    
    otai_deserialize_ocm_spectrum_power_list(j["spectrum_power_list"], list);

    if (!m_translator->tryTranslateRidToVid(rid, vid))
    {
        SWSS_LOG_ERROR("translate rid to vid failed, rid=0x%" PRIx64, rid);
        return;
    }

    linecard_vid = m_translator->translateRidToVid(linecard_rid, OTAI_NULL_OBJECT_ID);

    auto counters_db = std::shared_ptr<swss::DBConnector>(new swss::DBConnector("COUNTERS_DB", 0));
    std::string strVid = otai_serialize_object_id(vid);
    auto key = counters_db->hget(COUNTERS_OT_OCM_NAME_MAP, strVid);
    if (key == nullptr)
    {
        SWSS_LOG_ERROR("cannot get name map, %s %s", COUNTERS_OT_OCM_NAME_MAP, strVid.c_str());
        return;
    }

    for (uint32_t i = 0 ; i < list.count; i++)
    {
        std::string lowFreq = otai_serialize_number(list.list[i].lower_frequency);
        std::string upFreq = otai_serialize_number(list.list[i].upper_frequency);
        std::string power = otai_serialize_decimal(list.list[i].power);

        std::string tableKey = *key + '|' + lowFreq + '|' + upFreq;

        m_stateOcmTable->hset(tableKey, "lower-frequency", lowFreq);
        m_stateOcmTable->hset(tableKey, "upper-frequency", upFreq);
        m_stateOcmTable->hset(tableKey, "power", power);
        m_stateOcmTable->expire(tableKey, 60);  /* expire after 1 minute */
    }

    json j2;

    j2["linecard_id"] = otai_serialize_object_id(linecard_vid);
    j2["ocm_id"] = otai_serialize_object_id(vid);

    sendNotification(OTAI_OCM_NOTIFICATION_NAME_SPECTRUM_POWER_NOTIFY, j2.dump());
}

void writeOtdrTable(
        std::shared_ptr<Table> table,
        std::string &key,
        std::string &name,
        json &j)
{
    SWSS_LOG_ENTER();

    table->hset(key, "name", name);
    table->hset(key, "scan-time", j["scan-time"]);
    table->hset(key, "distance-range", j["distance-range"]);
    table->hset(key, "pulse-width", j["pulse-width"]);
    table->hset(key, "average-time", j["average-time"]);
    table->hset(key, "output-frequency", j["output-frequency"]);
    table->hset(key, "span-distance", j["span-distance"]);
    table->hset(key, "span-loss", j["span-loss"]);
    table->hset(key, "update-time", j["update-time"]);
    table->hset(key, "data", j["data"]);
}

void writeOtdrEventTable(
        std::shared_ptr<Table> table,
        std::string &key,
        otai_otdr_event_t &e,
        uint32_t index)
{
    SWSS_LOG_ENTER();

    table->hset(key, "index", otai_serialize_number(index));
    table->hset(key, "length", otai_serialize_decimal(e.length));
    table->hset(key, "loss", otai_serialize_decimal(e.loss));
    table->hset(key, "accumulate-loss", otai_serialize_decimal(e.accumulate_loss));
    table->hset(key, "type", otai_serialize_enum(e.type, &otai_metadata_enum_otai_otdr_event_type_t));
    table->hset(key, "reflection", otai_serialize_decimal(e.reflection));
}

void NotificationProcessor::handle_otdr_result_notify(
    _In_ const std::string& data,
    _In_ const std::vector<FieldValueTuple>& fv)
{
    SWSS_LOG_ENTER();

    otai_object_id_t otdrVid;
    otai_object_id_t otdrRid;

    json j = json::parse(data);

    otai_deserialize_object_id(j["otdr_id"], otdrRid);

    if (!m_translator->tryTranslateRidToVid(otdrRid, otdrVid))
    {
        SWSS_LOG_ERROR("translate rid to vid failed, rid=0x%" PRIx64, otdrRid);
        return;
    }

    auto counters_db = std::shared_ptr<swss::DBConnector>(new swss::DBConnector("COUNTERS_DB", 0));

    std::string strVid = otai_serialize_object_id(otdrVid);

    auto key = counters_db->hget(COUNTERS_OT_OTDR_NAME_MAP, strVid);

    if (key == nullptr)
    {
        SWSS_LOG_ERROR("cannot get name map, %s %s", COUNTERS_OT_OTDR_NAME_MAP, strVid.c_str());
        return;
    }

    std::string stateTableKey = *key + "|CURRENT";

    writeOtdrTable(m_stateOtdrTable, stateTableKey, *key, j);

    otai_otdr_event_list_t events;

    otai_deserialize_otdr_event_list(j["events"], events);

    for (uint32_t i = 0; i < events.count; i++)
    {
        uint32_t index = i + 1;

        std::string eventKey = *key + "|CURRENT|" + otai_serialize_number(index);

        writeOtdrEventTable(m_stateOtdrEventTable, eventKey, events.list[i], index);
    }

    std::string strScanTime = j["scan-time"];
    std::string historyTableKey = *key + "|" + strScanTime;

    writeOtdrTable(m_historyOtdrTable, historyTableKey, *key, j);

    for (uint32_t i = 0; i < events.count; i++)
    {
        uint32_t index = i + 1;

        std::string eventKey = *key + "|" + strScanTime + "|" + otai_serialize_number(index);

        writeOtdrEventTable(m_historyOtdrEventTable, eventKey, events.list[i], index);
    }

    uint64_t scanTime = 0;

    otai_deserialize_number(j["scan-time"], scanTime);

    m_otdrScanTimeQueue[*key].push(scanTime);

    while (m_otdrScanTimeQueue[*key].size() > 10)
    {
        scanTime = m_otdrScanTimeQueue[*key].front(); 

        m_otdrScanTimeQueue[*key].pop();

        std::string entry = m_historyOtdrTable->getTableName() + ":" +
                            *key + "|" + otai_serialize_number(scanTime);

        SWSS_LOG_INFO("Delete old otdr data, %s", entry.c_str());

        m_history_db->del(entry);

        std::string pattern = m_historyOtdrEventTable->getTableName() + ":" +
                              *key + "|" + otai_serialize_number(scanTime) + "*";

        auto keys = m_history_db->keys(pattern);

        for (auto &k : keys)
        {
            SWSS_LOG_INFO("Delete old otdr event data, %s", k.c_str());

            m_history_db->del(k);
        }
    }
}

void NotificationProcessor::handle_linecard_alarm(
    _In_ const std::string& data)
{
    SWSS_LOG_ENTER();

    otai_object_id_t linecard_id;
    otai_alarm_type_t alarm_type;
    otai_alarm_info_t alarm_info;

    otai_deserialize_linecard_alarm(data, linecard_id, alarm_type, alarm_info);
    std::lock_guard<std::mutex> lock_alarm(m_mtxAlarmTable);
    if (alarm_info.status == OTAI_ALARM_STATUS_ACTIVE)
    {
        handler_alarm_generated(data);
    }
    else if (alarm_info.status == OTAI_ALARM_STATUS_INACTIVE)
    {
        handler_alarm_cleared(data);
    }
    else
    {
        handler_event_generated(data);
    }
}

std::string NotificationProcessor::get_resource_name_by_rid(
        _In_ otai_object_id_t rid)
{
    SWSS_LOG_ENTER();

    otai_object_id_t vid;

    if (!m_translator->tryTranslateRidToVid(rid, vid))
    {
        SWSS_LOG_ERROR("Failed to translate rid to vid, rid=0x%" PRIx64, rid);

        return "";
    }

    auto counters_db = std::shared_ptr<swss::DBConnector>(new swss::DBConnector("COUNTERS_DB", 0));
    std::string strVid = otai_serialize_object_id(vid);
    auto key = counters_db->hget("VID2NAME", strVid);
    if (key == NULL)
    {
        SWSS_LOG_ERROR("Failed to get name from VID2NAME, vid=0x%" PRIx64, vid);

        return "";
    }

    return *key;
}

void NotificationProcessor::handler_event_generated(
    _In_ const std::string data)
{
    SWSS_LOG_ENTER();

    std::vector<FieldValueTuple> alarmVector;
    std::vector<FieldValueTuple> vectortemp;

    json j = json::parse(data);
    FieldValueTuple tupletemp;
    std::string keyid;

    std::string data_array[] = { "id","time-created","text","severity","type-id" };

    std::string resource, timecreated, type_id;

    otai_object_id_t rid;
    otai_deserialize_object_id(j["resource_oid"], rid);
    resource = get_resource_name_by_rid(rid);

    timecreated = j["time-created"];
    type_id = j["type-id"];

    keyid = j["id"] = resource + "#" + type_id;
    j["time-created"] = timecreated;

    for (uint32_t i = 0; i < sizeof(data_array) / sizeof(data_array[0]); i++)
    {
        tupletemp = std::make_pair(data_array[i], j[data_array[i]]);
        alarmVector.emplace_back(tupletemp);
    }

    tupletemp = std::make_pair("resource", resource);
    alarmVector.emplace_back(tupletemp);

    std::string strKey = keyid + "#" + timecreated;
    m_historyEventTable->set(strKey, alarmVector);
    m_historyEventTable->expire(strKey, m_ttlAlarm);
    SWSS_LOG_WARN("EVENT generated key:%s content:%s", strKey.c_str(), data.c_str());
}

void NotificationProcessor::handler_alarm_generated(
    _In_ const std::string data)
{
    SWSS_LOG_ENTER();

    std::vector<FieldValueTuple> alarmVector;
    std::vector<FieldValueTuple> vectortemp;

    bool alarmstatus;
    json j = json::parse(data);
    FieldValueTuple tupletemp;
    std::string keyid;

    std::string data_array[] = { "id","time-created","text","severity","type-id" };

    std::string resource, type_id, timecreated;

    otai_object_id_t rid;
    otai_deserialize_object_id(j["resource_oid"], rid);
    resource = get_resource_name_by_rid(rid);

    type_id = j["type-id"];
    timecreated = j["time-created"];

    keyid = j["id"] = resource + "#" + type_id;
    j["time-created"] = timecreated;

    alarmstatus = m_stateAlarmable->get(keyid, vectortemp);

    if (alarmstatus == true)
    {
        SWSS_LOG_NOTICE("alarm already generated(%s)", keyid.c_str());
        return;
    }

    for (uint32_t i = 0; i < sizeof(data_array) / sizeof(data_array[0]); i++)
    {
        tupletemp = std::make_pair(data_array[i], j[data_array[i]]);
        alarmVector.emplace_back(tupletemp);
    }
    tupletemp = std::make_pair("resource", resource);
    alarmVector.emplace_back(tupletemp);

    m_stateAlarmable->set(keyid, alarmVector);
    SWSS_LOG_WARN("ALARM generated key:%s content:%s", keyid.c_str(), data.c_str());
}

void NotificationProcessor::handler_history_alarm(
    _In_ const std::string& key,
    _In_ const std::string& timecreated,
    _In_ const std::vector<FieldValueTuple>& alarmvector)
{
    std::string strKey = key + "#" + timecreated;
    m_historyAlarmTable->set(strKey, alarmvector);
    m_historyAlarmTable->expire(strKey, m_ttlAlarm);
}

void NotificationProcessor::handler_alarm_cleared(
    _In_ const std::string data)
{
    SWSS_LOG_ENTER();

    std::string keyid;
    bool alarmstatus;
    std::vector<FieldValueTuple> vectortemp;
    std::string resource, type_id;

    json j = json::parse(data);

    otai_object_id_t rid;
    otai_deserialize_object_id(j["resource_oid"], rid);
    resource = get_resource_name_by_rid(rid);

    type_id = j["type-id"];

    keyid = resource + "#" + type_id;
    alarmstatus = m_stateAlarmable->get(keyid, vectortemp);

    if (alarmstatus == false)
    {
        SWSS_LOG_WARN("alarm already cleared(%s)", keyid.c_str());
        return;
    }
    else
    {
        std::string time_cleared = j["time-created"];//the attribute "time-created" is actually the time of alarm cleared.
        FieldValueTuple tupletemp = std::make_pair("time-cleared", time_cleared);
        vectortemp.push_back(tupletemp);
        handler_history_alarm(keyid, time_cleared, vectortemp);
        m_stateAlarmable->del(keyid);
        SWSS_LOG_WARN("ALARM cleared key:%s content:%s", keyid.c_str(), data.c_str());
    }
}

void NotificationProcessor::processNotification(
    _In_ const swss::KeyOpFieldsValuesTuple& item)
{
    SWSS_LOG_ENTER();

    m_synchronizer(item);
}

void NotificationProcessor::syncProcessNotification(
    _In_ const swss::KeyOpFieldsValuesTuple& item)
{
    SWSS_LOG_ENTER();

    std::string notification = kfvKey(item);
    std::string data = kfvOp(item);
    std::vector<FieldValueTuple> fv = kfvFieldsValues(item);

    if (notification == OTAI_LINECARD_NOTIFICATION_NAME_LINECARD_STATE_CHANGE)
    {
        handle_linecard_state_change(data);
    }
    else if (notification == OTAI_LINECARD_NOTIFICATION_NAME_LINECARD_ALARM_NOTIFY)
    {
        handle_linecard_alarm(data);
    }
    else if (notification == OTAI_APS_NOTIFICATION_NAME_OLP_SWITCH_NOTIFY)
    {
        handle_olp_switch_notify(data, fv);
    }
    else if (notification == OTAI_OCM_NOTIFICATION_NAME_SPECTRUM_POWER_NOTIFY)
    {
        handle_ocm_spectrum_power_notify(data, fv);
    }
    else if (notification == OTAI_OTDR_NOTIFICATION_NAME_RESULT_NOTIFY)
    {
        handle_otdr_result_notify(data, fv);
    }
    else
    {
        SWSS_LOG_ERROR("unknown notification: %s", notification.c_str());
    }
}

void NotificationProcessor::ntf_process_function()
{
    SWSS_LOG_ENTER();

    std::mutex ntf_mutex;

    std::unique_lock<std::mutex> ulock(ntf_mutex);

    while (m_runThread)
    {
        m_cv.wait(ulock);

        // this is notifications processing thread context, which is different
        // from OTAI notifications context, we can safe use syncd mutex here,
        // processing each notification is under same mutex as processing main
        // events, counters and reinit

        swss::KeyOpFieldsValuesTuple item;

        while (m_notificationQueue->tryDequeue(item))
        {
            processNotification(item);
        }
    }
}

void NotificationProcessor::startNotificationsProcessingThread()
{
    SWSS_LOG_ENTER();

    m_runThread = true;

    m_ntf_process_thread = std::make_shared<std::thread>(&NotificationProcessor::ntf_process_function, this);
}

void NotificationProcessor::stopNotificationsProcessingThread()
{
    SWSS_LOG_ENTER();

    m_runThread = false;

    m_cv.notify_all();

    if (m_ntf_process_thread != nullptr)
    {
        m_ntf_process_thread->join();
    }

    m_ntf_process_thread = nullptr;
}

void NotificationProcessor::signal()
{
    SWSS_LOG_ENTER();

    m_cv.notify_all();
}

std::shared_ptr<NotificationQueue> NotificationProcessor::getQueue() const
{
    SWSS_LOG_ENTER();

    return m_notificationQueue;
}
