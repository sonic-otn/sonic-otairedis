#include "NotificationHandler.h"
#include "lairediscommon.h"

#include "swss/logger.h"
#include "swss/json.hpp"

#include "meta/lai_serialize.h"
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
    m_notificationQueue = processor->getQueue();
}

NotificationHandler::~NotificationHandler()
{
    SWSS_LOG_ENTER();

    // empty
}

void NotificationHandler::setLinecardNotifications(
    _In_ const lai_notifications_t& linecardNotifications)
{
    SWSS_LOG_ENTER();

    m_notifications = linecardNotifications;
}

const lai_notifications_t& NotificationHandler::getLinecardNotifications() const
{
    SWSS_LOG_ENTER();

    return m_notifications;
}

void NotificationHandler::onApsReportSwitchInfo(
    _In_ lai_object_id_t rid,
    _In_ lai_olp_switch_t switch_info)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("APS report switch info, rid=%s", lai_serialize_object_id(rid).c_str());

    int number = switch_info.num < 10 ? switch_info.num : 10;

    for (int i = 0; i < number; i++)
    {
        nlohmann::json j;
        std::vector<swss::FieldValueTuple> fv;

        lai_olp_switch_info_t &info = switch_info.info[i];

        j["rid"] = lai_serialize_object_id(rid);
        j["time-stamp"] = lai_serialize_number(info.time_stamp);

        fv.push_back(FieldValueTuple("rid", lai_serialize_object_id(rid)));
        fv.push_back(FieldValueTuple("time-stamp", lai_serialize_number(info.time_stamp)));
        fv.push_back(FieldValueTuple("index", lai_serialize_number(info.index)));
        fv.push_back(FieldValueTuple("interval", lai_serialize_number(switch_info.interval)));
        fv.push_back(FieldValueTuple("pointers", lai_serialize_number(switch_info.pointers)));
        fv.push_back(FieldValueTuple("reason", lai_serialize_enum_v2(info.reason, &lai_metadata_enum_lai_olp_switch_reason_t)));
        fv.push_back(FieldValueTuple("operate",  lai_serialize_enum_v2(info.operate, &lai_metadata_enum_lai_olp_switch_operate_t)));
        fv.push_back(FieldValueTuple("switching-common_out", lai_serialize_decimal(info.switching.common_out)));
        fv.push_back(FieldValueTuple("switching-primary_in", lai_serialize_decimal(info.switching.primary_in)));
        fv.push_back(FieldValueTuple("switching-secondary_in", lai_serialize_decimal(info.switching.secondary_in)));

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
        
            fv.push_back(FieldValueTuple(before_common_out, lai_serialize_decimal(info.before[p].common_out)));
            fv.push_back(FieldValueTuple(before_primary_in, lai_serialize_decimal(info.before[p].primary_in)));
            fv.push_back(FieldValueTuple(before_secondary_in, lai_serialize_decimal(info.before[p].secondary_in)));
            fv.push_back(FieldValueTuple(after_common_out, lai_serialize_decimal(info.after[p].common_out)));
            fv.push_back(FieldValueTuple(after_primary_in, lai_serialize_decimal(info.after[p].primary_in)));
            fv.push_back(FieldValueTuple(after_secondary_in, lai_serialize_decimal(info.after[p].secondary_in)));
        }

        std::string s = j.dump();
        enqueueNotification(LAI_APS_NOTIFICATION_NAME_OLP_SWITCH_NOTIFY, s, fv);
    } 
}

void NotificationHandler::onOcmReportSpectrumPower(
    _In_ lai_object_id_t linecard_rid,
    _In_ lai_object_id_t ocm_id,
    _In_ lai_spectrum_power_list_t ocm_result)
{
    SWSS_LOG_ENTER();

    if (ocm_result.list == NULL)
    {
        return;
    }

    nlohmann::json j;
    j["linecard_rid"] = lai_serialize_object_id(linecard_rid);
    j["ocm_id"] = lai_serialize_object_id(ocm_id);
    j["spectrum_power_list"] = lai_serialize_ocm_spectrum_power_list(ocm_result);

    std::string s = j.dump();
    enqueueNotification(LAI_OCM_NOTIFICATION_NAME_SPECTRUM_POWER_NOTIFY, s);
}

void NotificationHandler::onOtdrReportResult(
    _In_ lai_object_id_t linecard_rid,
    _In_ lai_object_id_t otdr_id,
    _In_ lai_otdr_result_t result)
{
    SWSS_LOG_ENTER();

    nlohmann::json j;

    j["linecard_rid"] = lai_serialize_object_id(linecard_rid);
    j["otdr_id"] = lai_serialize_object_id(otdr_id);

    j["scan-time"] = lai_serialize_number(result.scanning_profile.scan_time);
    j["distance-range"] = lai_serialize_number(result.scanning_profile.distance_range);
    j["pulse-width"] = lai_serialize_number(result.scanning_profile.pulse_width);
    j["average-time"] = lai_serialize_number(result.scanning_profile.average_time);
    j["output-frequency"] = lai_serialize_number(result.scanning_profile.output_frequency);

    j["span-distance"] = lai_serialize_decimal(result.events.span_distance);
    j["span-loss"] = lai_serialize_decimal(result.events.span_loss);
    j["events"] = lai_serialize_otdr_event_list(result.events.events);

    j["update-time"] = lai_serialize_number(result.trace.update_time);
    j["data"] = lai_serialize_hex_binary(result.trace.data.list, result.trace.data.count);

    std::string s = j.dump();
    enqueueNotification(LAI_OTDR_NOTIFICATION_NAME_RESULT_NOTIFY, s);
}

void NotificationHandler::updateNotificationsPointers(
    _In_ lai_object_type_t object_type,
    _In_ uint32_t attr_count,
    _In_ lai_attribute_t* attr_list) const
{
    SWSS_LOG_ENTER();

    /*
     * This function should only be called on CREATE/SET api when object is
     * LINECARD.
     *
     * Notifications pointers needs to be corrected since those we receive from
     * lairedis are in lairedis memory space and here we are using those ones
     * we declared in syncd memory space.
     *
     * Also notice that we are using the same pointers for ALL linecards.
     */

    for (uint32_t index = 0; index < attr_count; ++index)
    {
        lai_attribute_t& attr = attr_list[index];

        auto meta = lai_metadata_get_attr_metadata(object_type, attr.id);

        if (meta->attrvaluetype != LAI_ATTR_VALUE_TYPE_POINTER)
        {
            continue;
        }

        /*
         * Does not matter if pointer is valid or not, we just want the
         * previous value.
         */

        lai_pointer_t prev = attr.value.ptr;

        if (prev == NULL)
        {
            /*
             * If pointer is NULL, then fine, let it be.
             */

            continue;
        }

        if (object_type == LAI_OBJECT_TYPE_LINECARD)
        {
            switch (attr.id)
            {
            case LAI_LINECARD_ATTR_LINECARD_STATE_CHANGE_NOTIFY:
                attr.value.ptr = (void*)m_notifications.on_linecard_state_change;
                break;

            case LAI_LINECARD_ATTR_LINECARD_ALARM_NOTIFY:
                attr.value.ptr = (void*)m_notifications.on_linecard_alarm;
                break;

            case LAI_LINECARD_ATTR_LINECARD_OCM_SPECTRUM_POWER_NOTIFY:
                attr.value.ptr = (void*)m_notifications.on_linecard_ocm_spectrum_power;
                break;

            case LAI_LINECARD_ATTR_LINECARD_OTDR_RESULT_NOTIFY:
                attr.value.ptr = (void*)m_notifications.on_linecard_otdr_result;
                break;

            default:
                SWSS_LOG_ERROR("pointer for %s is not handled, FIXME!", meta->attridname);
                continue;
            }
        }
        else if (object_type == LAI_OBJECT_TYPE_APS)
        {
            switch (attr.id)
            {
            case LAI_APS_ATTR_SWITCH_INFO_NOTIFY:
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
    _In_ lai_object_id_t linecard_rid,
    _In_ lai_oper_status_t linecard_oper_status)
{
    SWSS_LOG_ENTER();

    auto s = lai_serialize_linecard_oper_status(linecard_rid, linecard_oper_status);

    enqueueNotification(LAI_LINECARD_NOTIFICATION_NAME_LINECARD_STATE_CHANGE, s);

    generate_linecard_communication_alarm(linecard_rid, linecard_oper_status);
}

void NotificationHandler::generate_linecard_communication_alarm(
    _In_ lai_object_id_t linecard_rid,
    _In_ lai_oper_status_t linecard_oper_status)
{
    SWSS_LOG_ENTER();

    std::vector<std::string> linecardkey;
    m_linecardtable->getKeys(linecardkey);
    for (const auto& strlinecard : linecardkey)
    {
        nlohmann::json j;

        j["linecard_id"] = lai_serialize_object_id(linecard_rid);

        j["time-created"] = lai_serialize_number((uint64_t)chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch()).count());

        j["resource_oid"] = lai_serialize_object_id(linecard_rid);
        j["id"] = strlinecard;
        j["text"] = "SLOT COMMUNICATION FAIL";

        j["severity"] = lai_serialize_enum_v2(LAI_ALARM_SEVERITY_CRITICAL, &lai_metadata_enum_lai_alarm_severity_t);
        j["type-id"] = lai_serialize_enum_v2(LAI_ALARM_TYPE_SLOT_COMM_FAIL, &lai_metadata_enum_lai_alarm_type_t);

        lai_alarm_status_t status = LAI_ALARM_STATUS_INACTIVE;

        if (linecard_oper_status == LAI_OPER_STATUS_INACTIVE)
        {
            status = LAI_ALARM_STATUS_ACTIVE;
            m_linecardtable->hset(strlinecard, "slot-status", "CommFail");
        }
        else
        {
            m_linecardtable->hset(strlinecard, "slot-status", "Ready");
        }
        j["status"] = lai_serialize_enum(status, &lai_metadata_enum_lai_alarm_status_t);

        std::string s = j.dump();
        enqueueNotification(LAI_LINECARD_NOTIFICATION_NAME_LINECARD_ALARM_NOTIFY, s);
    }
}

void NotificationHandler::onLinecardAlarm(
    _In_ lai_object_id_t linecard_rid,
    _In_ lai_alarm_type_t alarm_type,
    _In_ lai_alarm_info_t alarm_info)
{
    SWSS_LOG_ENTER();

    nlohmann::json j;
    std::string str_resource, type_id, str_text, s;

    if ((uint32_t)alarm_type >= LAI_ALARM_TYPE_MAX)
    {
        SWSS_LOG_ERROR("error alarm type %d (%s)\n", alarm_type, s.c_str());
        return;
    }
    j["linecard_id"] = lai_serialize_object_id(linecard_rid);
    j["time-created"] = lai_serialize_number(alarm_info.time_created);

    j["resource_oid"] = lai_serialize_object_id(alarm_info.resource_oid);

    if (alarm_info.text.list == NULL)
        j["text"] = "";
    else
        j["text"] = str_text = (char*)alarm_info.text.list;


    j["severity"] = lai_serialize_enum_v2(alarm_info.severity, &lai_metadata_enum_lai_alarm_severity_t);
    j["type-id"] = lai_serialize_enum_v2(alarm_type, &lai_metadata_enum_lai_alarm_type_t);
    j["status"] = lai_serialize_enum(alarm_info.status, &lai_metadata_enum_lai_alarm_status_t);
    j["id"] = str_resource + "#" + type_id;
    s = j.dump();

    SWSS_LOG_INFO("alarm(%s)\n", s.c_str());  //to be delete

    enqueueNotification(LAI_LINECARD_NOTIFICATION_NAME_LINECARD_ALARM_NOTIFY, s);
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

