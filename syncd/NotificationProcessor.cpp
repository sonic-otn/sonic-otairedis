#include "NotificationProcessor.h"
#include "RedisClient.h"


#include "meta/lai_serialize.h"
#include "meta/LaiAttributeList.h"

#include "swss/logger.h"
#include "swss/notificationproducer.h"

#include "swss/json.hpp"
#include <inttypes.h>
#include "Common.h"

using json = nlohmann::json;
using namespace syncd;
using namespace laimeta;
using namespace swss;


NotificationProcessor::NotificationProcessor(
    _In_ std::mutex& mtxAlarm,
    _In_ std::shared_ptr<NotificationProducerBase> producer,
    _In_ std::string dbAsic,
    _In_ std::shared_ptr<RedisClient> client,
    _In_ std::function<void(const swss::KeyOpFieldsValuesTuple&)> synchronizer,
    _In_ std::function<void(const lai_oper_status_t&)> linecard_state_change_handler) :
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

    m_history_db = std::shared_ptr<DBConnector>(new DBConnector("HISTORY_DB", 0));
    m_historyAlarmable = std::unique_ptr<Table>(new Table(m_history_db.get(), "HISALARM"));
    m_historyEventable = std::unique_ptr<Table>(new Table(m_history_db.get(), "HISEVENT"));

    m_ttlPM15Min = EXIPRE_TIME_SECONDS_2DAYS;
    m_ttlPM24Hour = EXIPRE_TIME_SECONDS_7DAYS;
    m_ttlAlarm = EXIPRE_TIME_SECONDS_7DAYS;
}

NotificationProcessor::~NotificationProcessor()
{
    SWSS_LOG_ENTER();

    stopNotificationsProcessingThread();
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
    _In_ lai_object_id_t linecard_rid,
    _In_ lai_oper_status_t linecard_oper_status)
{
    SWSS_LOG_ENTER();

    lai_object_id_t linecard_vid = m_translator->translateRidToVid(linecard_rid, LAI_NULL_OBJECT_ID);

    auto s = lai_serialize_linecard_oper_status(linecard_vid, linecard_oper_status);

    SWSS_LOG_NOTICE("linecard state change to %s", s.c_str());

    sendNotification(LAI_LINECARD_NOTIFICATION_NAME_LINECARD_STATE_CHANGE, s);

    swss::DBConnector db(m_dbAsic, 0);
    swss::NotificationProducer linecard_state(&db, SYNCD_NOTIFICATION_CHANNEL_LINECARDSTATE);

    std::vector<swss::FieldValueTuple> values;
    linecard_state.send(s, s, values);
}

void NotificationProcessor::handle_linecard_state_change(
    _In_ const std::string& data)
{
    SWSS_LOG_ENTER();

    lai_oper_status_t linecard_oper_status;
    lai_object_id_t linecard_id;

    lai_deserialize_linecard_oper_status(data, linecard_id, linecard_oper_status);

    process_on_linecard_state_change(linecard_id, linecard_oper_status);
}

void NotificationProcessor::handle_olp_switch_notify(
    _In_ const std::string& data,
    _In_ const std::vector<FieldValueTuple>& fv)
{
    SWSS_LOG_ENTER();

    json j = json::parse(data);

    lai_object_id_t vid;
    lai_object_id_t rid;

    lai_deserialize_object_id(j["rid"], rid);

    if (!m_translator->tryTranslateRidToVid(rid, vid))
    {
        SWSS_LOG_ERROR("translate rid to vid failed, rid=0x%" PRIx64, rid);
        return;
    }
    auto counters_db = std::shared_ptr<swss::DBConnector>(new swss::DBConnector("COUNTERS_DB", 0));
    std::string strVid = lai_serialize_object_id(vid);
    auto key = counters_db->hget(COUNTERS_APS_NAME_MAP, strVid);
    if (key == nullptr)
    {
        SWSS_LOG_ERROR("cannot get name map, %s %s", COUNTERS_APS_NAME_MAP, strVid.c_str());
        return;
    }
    std::string strKey = *key;
    strKey += "_";
    strKey += j["time-stamp"];

    SWSS_LOG_NOTICE("record olp switch info, key=%s", strKey.c_str());

    m_stateOLPSwitchInfoTbl->set(strKey, fv);
}

void NotificationProcessor::handle_linecard_alarm(
    _In_ const std::string& data)
{
    SWSS_LOG_ENTER();

    lai_object_id_t linecard_id;
    lai_alarm_type_t alarm_type;
    lai_alarm_info_t alarm_info;

    lai_deserialize_linecard_alarm(data, linecard_id, alarm_type, alarm_info);
    std::lock_guard<std::mutex> lock_alarm(m_mtxAlarmTable);
    if (alarm_info.status == LAI_ALARM_STATUS_ACTIVE)
    {
        handler_alarm_generated(data);
    }
    else if (alarm_info.status == LAI_ALARM_STATUS_INACTIVE)
    {
        handler_alarm_cleared(data);
    }
    else
    {
        handler_event_generated(data);
    }
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

    std::string dataarry[] = { "id","time-created","resource","text","severity","type-id" };

    std::string resource, timecreated, type_id;

    resource = j["resource"];
    timecreated = j["time-created"];
    type_id = j["type-id"];

    keyid = j["id"] = resource + "#" + type_id;
    j["time-created"] = timecreated;

    for (uint32_t i = 0; i < sizeof(dataarry) / sizeof(dataarry[0]); i++)
    {
        tupletemp = std::make_pair(dataarry[i], j[dataarry[i]]);
        alarmVector.emplace_back(tupletemp);
    }
    std::string strKey = keyid + "#" + timecreated;
    m_historyEventable->set(strKey, alarmVector);
    m_historyEventable->expire(strKey, m_ttlAlarm);
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

    std::string dataarry[] = { "id","time-created","resource","text","severity","type-id" };

    std::string resource, type_id, timecreated;

    resource = j["resource"];
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

    for (uint32_t i = 0; i < sizeof(dataarry) / sizeof(dataarry[0]); i++)
    {
        tupletemp = std::make_pair(dataarry[i], j[dataarry[i]]);
        alarmVector.emplace_back(tupletemp);
    }

    m_stateAlarmable->set(keyid, alarmVector);
    SWSS_LOG_WARN("ALARM generated key:%s content:%s", keyid.c_str(), data.c_str());
}

void NotificationProcessor::handler_history_alarm(
    _In_ const std::string& key,
    _In_ const std::string& timecreated,
    _In_ const std::vector<FieldValueTuple>& alarmvector)
{
    std::string strKey = key + "#" + timecreated;
    m_historyAlarmable->set(strKey, alarmvector);
    m_historyAlarmable->expire(strKey, m_ttlAlarm);
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

    resource = j["resource"];
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

    if (notification == LAI_LINECARD_NOTIFICATION_NAME_LINECARD_STATE_CHANGE)
    {
        handle_linecard_state_change(data);
    }
    else if (notification == LAI_LINECARD_NOTIFICATION_NAME_LINECARD_ALARM_NOTIFY)
    {
        handle_linecard_alarm(data);
    }
    else if (notification == LAI_APS_NOTIFICATION_NAME_OLP_SWITCH_NOTIFY)
    {
        handle_olp_switch_notify(data, fv);
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
        // from LAI notifications context, we can safe use syncd mutex here,
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
