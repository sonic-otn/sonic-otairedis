#include "NotificationHandler.h"
#include "otairediscommon.h"

#include "swss/logger.h"
#include "nlohmann/json.hpp"

#include "meta/otai_serialize.h"
#include <chrono>
#include <inttypes.h>

using namespace syncd;
using namespace std;
using namespace swss;

NotificationHandler::NotificationHandler(
    _In_ std::shared_ptr<NotificationProcessor> processor) :
    m_processor(processor)
{
    SWSS_LOG_ENTER();

    memset(&m_notifications, 0, sizeof(m_notifications));
    m_state_db = std::shared_ptr<DBConnector>(new DBConnector("STATE_DB", 0));
    m_linecardtable = std::unique_ptr<Table>(new Table(m_state_db.get(), "LINECARD"));
    m_curalarmtable = std::unique_ptr<Table>(new Table(m_state_db.get(), "CURALARM"));
    m_notificationQueue = processor->getQueue();
}

NotificationHandler::~NotificationHandler()
{
    SWSS_LOG_ENTER();

    // empty
}

void NotificationHandler::setLinecardNotifications(
    _In_ const otai_notifications_t& linecardNotifications)
{
    SWSS_LOG_ENTER();

    m_notifications = linecardNotifications;
}

const otai_notifications_t& NotificationHandler::getLinecardNotifications() const
{
    SWSS_LOG_ENTER();

    return m_notifications;
}

void NotificationHandler::onApsReportSwitchInfo(
    _In_ otai_object_id_t rid,
    _In_ otai_olp_switch_t switch_info)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("APS report switch info, rid=%s", otai_serialize_object_id(rid).c_str());

    int number = switch_info.num < 10 ? switch_info.num : 10;

    for (int i = 0; i < number; i++)
    {
        nlohmann::json j;
        std::vector<swss::FieldValueTuple> fv;

        otai_olp_switch_info_t &info = switch_info.info[i];

        j["rid"] = otai_serialize_object_id(rid);
        j["time-stamp"] = otai_serialize_number(info.time_stamp);

        fv.push_back(FieldValueTuple("rid", otai_serialize_object_id(rid)));
        fv.push_back(FieldValueTuple("time-stamp", otai_serialize_number(info.time_stamp)));
        fv.push_back(FieldValueTuple("index", otai_serialize_number(info.index)));
        fv.push_back(FieldValueTuple("interval", otai_serialize_number(switch_info.interval)));
        fv.push_back(FieldValueTuple("pointers", otai_serialize_number(switch_info.pointers)));
        fv.push_back(FieldValueTuple("reason", otai_serialize_enum_v2(info.reason, &otai_metadata_enum_otai_olp_switch_reason_t)));
        fv.push_back(FieldValueTuple("operate",  otai_serialize_enum_v2(info.operate, &otai_metadata_enum_otai_olp_switch_operate_t)));
        fv.push_back(FieldValueTuple("switching-common_out", otai_serialize_decimal(info.switching.common_out)));
        fv.push_back(FieldValueTuple("switching-primary_in", otai_serialize_decimal(info.switching.primary_in)));
        fv.push_back(FieldValueTuple("switching-secondary_in", otai_serialize_decimal(info.switching.secondary_in)));

        int pointers = ((switch_info.pointers - 1) / 2);
        if (pointers > 40) {
            pointers = 40;
        }

        for (int p = 0; p < pointers; p++)
        {
            string before_common_out = "before-" + to_string(p+1) + "-common_out";
            string before_primary_in = "before-" + to_string(p+1) + "-primary_in";
            string before_secondary_in = "before-" + to_string(p+1) + "-secondary_in";
            string after_common_out = "after-" + to_string(p+1) + "-common_out";
            string after_primary_in = "after-" + to_string(p+1) + "-primary_in";
            string after_secondary_in = "after-" + to_string(p+1) + "-secondary_in";
        
            fv.push_back(FieldValueTuple(before_common_out, otai_serialize_decimal(info.before[p].common_out)));
            fv.push_back(FieldValueTuple(before_primary_in, otai_serialize_decimal(info.before[p].primary_in)));
            fv.push_back(FieldValueTuple(before_secondary_in, otai_serialize_decimal(info.before[p].secondary_in)));
            fv.push_back(FieldValueTuple(after_common_out, otai_serialize_decimal(info.after[p].common_out)));
            fv.push_back(FieldValueTuple(after_primary_in, otai_serialize_decimal(info.after[p].primary_in)));
            fv.push_back(FieldValueTuple(after_secondary_in, otai_serialize_decimal(info.after[p].secondary_in)));
        }

        std::string s = j.dump();
        enqueueNotification(OTAI_APS_NOTIFICATION_NAME_OLP_SWITCH_NOTIFY, s, fv);
    } 
}

void NotificationHandler::onOcmReportSpectrumPower(
    _In_ otai_object_id_t linecard_rid,
    _In_ otai_object_id_t ocm_id,
    _In_ otai_spectrum_power_list_t ocm_result)
{
    SWSS_LOG_ENTER();

    if (ocm_result.list == NULL)
    {
        return;
    }

    nlohmann::json j;
    j["linecard_rid"] = otai_serialize_object_id(linecard_rid);
    j["ocm_id"] = otai_serialize_object_id(ocm_id);
    j["spectrum_power_list"] = otai_serialize_ocm_spectrum_power_list(ocm_result);

    std::string s = j.dump();
    enqueueNotification(OTAI_OCM_NOTIFICATION_NAME_SPECTRUM_POWER_NOTIFY, s);
}

void NotificationHandler::onOtdrReportResult(
    _In_ otai_object_id_t linecard_rid,
    _In_ otai_object_id_t otdr_id,
    _In_ otai_otdr_result_t result)
{
    SWSS_LOG_ENTER();

    nlohmann::json j;

    j["linecard_rid"] = otai_serialize_object_id(linecard_rid);
    j["otdr_id"] = otai_serialize_object_id(otdr_id);

    j["scan-time"] = otai_serialize_number(result.scanning_profile.scan_time);
    j["distance-range"] = otai_serialize_number(result.scanning_profile.distance_range);
    j["pulse-width"] = otai_serialize_number(result.scanning_profile.pulse_width);
    j["average-time"] = otai_serialize_number(result.scanning_profile.average_time);
    j["output-frequency"] = otai_serialize_number(result.scanning_profile.output_frequency);

    j["span-distance"] = otai_serialize_decimal(result.events.span_distance);
    j["span-loss"] = otai_serialize_decimal(result.events.span_loss);
    j["events"] = otai_serialize_otdr_event_list(result.events.events);

    j["update-time"] = otai_serialize_number(result.trace.update_time);
    j["data"] = otai_serialize_hex_binary(result.trace.data.list, result.trace.data.count);

    std::string s = j.dump();
    enqueueNotification(OTAI_OTDR_NOTIFICATION_NAME_RESULT_NOTIFY, s);
}

void NotificationHandler::updateNotificationsPointers(
    _In_ otai_object_type_t object_type,
    _In_ uint32_t attr_count,
    _In_ otai_attribute_t* attr_list) const
{
    SWSS_LOG_ENTER();
    SWSS_LOG_NOTICE("update object:%s notification pointers",
                otai_serialize_object_type(object_type).c_str());
    /*
     * This function should only be called on CREATE/SET api when object is
     * LINECARD.
     *
     * Notifications pointers needs to be corrected since those we receive from
     * otairedis are in otairedis memory space and here we are using those ones
     * we declared in syncd memory space.
     *
     * Also notice that we are using the same pointers for ALL linecards.
     */

    for (uint32_t index = 0; index < attr_count; ++index)
    {
        otai_attribute_t& attr = attr_list[index];

        auto meta = otai_metadata_get_attr_metadata(object_type, attr.id);

        if (meta->attrvaluetype != OTAI_ATTR_VALUE_TYPE_POINTER)
        {
            continue;
        }

        /*
         * Does not matter if pointer is valid or not, we just want the
         * previous value.
         */

        otai_pointer_t prev = attr.value.ptr;

        if (prev == NULL)
        {
            /*
             * If pointer is NULL, then fine, let it be.
             */

            continue;
        }

        if (object_type == OTAI_OBJECT_TYPE_LINECARD)
        {
            switch (attr.id)
            {
            case OTAI_LINECARD_ATTR_LINECARD_STATE_CHANGE_NOTIFY:
                attr.value.ptr = (void*)m_notifications.on_linecard_state_change;
                break;

            case OTAI_LINECARD_ATTR_LINECARD_ALARM_NOTIFY:
                attr.value.ptr = (void*)m_notifications.on_linecard_alarm;
                break;

            case OTAI_LINECARD_ATTR_LINECARD_OCM_SPECTRUM_POWER_NOTIFY:
                attr.value.ptr = (void*)m_notifications.on_linecard_ocm_spectrum_power;
                break;

            case OTAI_LINECARD_ATTR_LINECARD_OTDR_RESULT_NOTIFY:
                attr.value.ptr = (void*)m_notifications.on_linecard_otdr_result;
                break;

            default:
                SWSS_LOG_ERROR("pointer for %s is not handled, FIXME!", meta->attridname);
                continue;
            }
        }
        else if (object_type == OTAI_OBJECT_TYPE_APS)
        {
            switch (attr.id)
            {
            case OTAI_APS_ATTR_SWITCH_INFO_NOTIFY:
                attr.value.ptr = (void*)m_notifications.on_aps_report_switch_info;
                break;

            default:

                SWSS_LOG_ERROR("pointer for %s is not handled, FIXME!", meta->attridname);
                continue;
            }
        
        }
        // Here we translated pointer, just log it.

        SWSS_LOG_INFO("%s: 0x%" PRIx64 " (orch) => 0x%" PRIx64 " (syncd)", meta->attridname, (uint64_t)prev, (uint64_t)attr.value.ptr);
    }
}

void NotificationHandler::onLinecardStateChange(
    _In_ otai_object_id_t linecard_rid,
    _In_ otai_oper_status_t linecard_oper_status)
{
    SWSS_LOG_ENTER();

    auto s = otai_serialize_linecard_oper_status(linecard_rid, linecard_oper_status);

    enqueueNotification(OTAI_LINECARD_NOTIFICATION_NAME_LINECARD_STATE_CHANGE, s);

    generate_linecard_communication_alarm(linecard_rid, linecard_oper_status);
}

void NotificationHandler::generate_linecard_communication_alarm(
    _In_ otai_object_id_t linecard_rid,
    _In_ otai_oper_status_t linecard_oper_status)
{
    SWSS_LOG_ENTER();

    std::vector<std::string> linecardkey;
    m_linecardtable->getKeys(linecardkey);
    for (const auto& strlinecard : linecardkey)
    {
        nlohmann::json j;

        j["linecard_id"] = otai_serialize_object_id(linecard_rid);

        j["time-created"] = otai_serialize_number((uint64_t)chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch()).count());

        j["resource_oid"] = otai_serialize_object_id(linecard_rid);
        j["id"] = strlinecard;
        j["text"] = "SLOT COMMUNICATION FAIL";

        j["severity"] = otai_serialize_enum_v2(OTAI_ALARM_SEVERITY_CRITICAL, &otai_metadata_enum_otai_alarm_severity_t);
        j["type-id"] = otai_serialize_enum_v2(OTAI_ALARM_TYPE_SLOT_COMM_FAIL, &otai_metadata_enum_otai_alarm_type_t);

        otai_alarm_status_t status = OTAI_ALARM_STATUS_INACTIVE;

        if (linecard_oper_status == OTAI_OPER_STATUS_INACTIVE)
        {
            status = OTAI_ALARM_STATUS_ACTIVE;
            m_linecardtable->hset(strlinecard, "slot-status", "CommFail");

            //flush all current alarms
            m_curalarmtable->flush();
        }
        else
        {
            m_linecardtable->hset(strlinecard, "slot-status", "Ready");
        }
        j["status"] = otai_serialize_enum(status, &otai_metadata_enum_otai_alarm_status_t);

        std::string s = j.dump();
        enqueueNotification(OTAI_LINECARD_NOTIFICATION_NAME_LINECARD_ALARM_NOTIFY, s);
    }
}

void NotificationHandler::onLinecardAlarm(
    _In_ otai_object_id_t linecard_rid,
    _In_ otai_alarm_type_t alarm_type,
    _In_ otai_alarm_info_t alarm_info)
{
    SWSS_LOG_ENTER();

    nlohmann::json j;
    std::string str_resource, type_id, str_text, s;

    if ((uint32_t)alarm_type >= OTAI_ALARM_TYPE_MAX)
    {
        SWSS_LOG_ERROR("error alarm type %d (%s)\n", alarm_type, s.c_str());
        return;
    }
    j["linecard_id"] = otai_serialize_object_id(linecard_rid);
    j["time-created"] = otai_serialize_number(alarm_info.time_created);

    j["resource_oid"] = otai_serialize_object_id(alarm_info.resource_oid);

    if (alarm_info.text.list == NULL)
        j["text"] = "";
    else
        j["text"] = str_text = (char*)alarm_info.text.list;


    j["severity"] = otai_serialize_enum_v2(alarm_info.severity, &otai_metadata_enum_otai_alarm_severity_t);
    j["type-id"] = otai_serialize_enum_v2(alarm_type, &otai_metadata_enum_otai_alarm_type_t);
    j["status"] = otai_serialize_enum(alarm_info.status, &otai_metadata_enum_otai_alarm_status_t);
    j["id"] = str_resource + "#" + type_id;
    s = j.dump();

    SWSS_LOG_INFO("alarm(%s)\n", s.c_str());  //to be delete

    enqueueNotification(OTAI_LINECARD_NOTIFICATION_NAME_LINECARD_ALARM_NOTIFY, s);
}

void NotificationHandler::enqueueNotification(
    _In_ const std::string& op,
    _In_ const std::string& data,
    _In_ const std::vector<swss::FieldValueTuple>& entry)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("%s %s", op.c_str(), data.c_str());

    swss::KeyOpFieldsValuesTuple item(op, data, entry);

    if (m_notificationQueue->enqueue(item))
    {
        m_processor->signal();
    }
}

void NotificationHandler::enqueueNotification(
    _In_ const std::string& op,
    _In_ const std::string& data)
{
    SWSS_LOG_ENTER();

    std::vector<swss::FieldValueTuple> entry;

    enqueueNotification(op, data, entry);
}

