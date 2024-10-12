#include <nlohmann/json.hpp>

#include "swss/logger.h"
#include "OtaiObjectNotificationSim.h"

using namespace std;
using namespace otaivs;
using namespace swss;
using json = nlohmann::json;

void OtaiObjectNotificationSim::triggerLinecardNtfs(otai_object_id_t linecard_id)
{
    m_linecardNotifThread = thread(&OtaiObjectNotificationSim::sendLinecardNotifications, this, linecard_id);
}

void OtaiObjectNotificationSim::triggerOcmScanNtfs(otai_object_id_t linecard_id, otai_object_id_t ocm_id)
{
    m_ocmNotifThread = thread(&OtaiObjectNotificationSim::sendOcmScanNotification, this, linecard_id, ocm_id);
}

void OtaiObjectNotificationSim::triggerOtdrScanNtfs(otai_object_id_t linecard_id, otai_object_id_t otdr_id)
{
    m_otdrNotifThread = thread(&OtaiObjectNotificationSim::sendOtdrScanNotification, this, linecard_id, otdr_id);
}

void OtaiObjectNotificationSim::triggerApsSwitchNtfs(otai_object_id_t aps_id)
{
    m_apsNotifThread = thread(&OtaiObjectNotificationSim::sendApsSwitchNotification, this, aps_id);
}

void OtaiObjectNotificationSim::updateNotificationCallback(
    _In_ uint32_t attr_count,
    _In_ const otai_attribute_t *attr_list)
{
    for (uint32_t idx = 0; idx < attr_count; ++idx)
    {
        otai_attr_id_t id = attr_list[idx].id;

        if(OTAI_LINECARD_ATTR_LINECARD_ALARM_NOTIFY == id) {
            m_alarmCallback = (otai_linecard_alarm_notification_fn)attr_list[idx].value.ptr;
        }

        if(OTAI_LINECARD_ATTR_LINECARD_STATE_CHANGE_NOTIFY == id) {
            m_stateChgCallback = (otai_linecard_state_change_notification_fn)attr_list[idx].value.ptr;
        }

        if(OTAI_LINECARD_ATTR_LINECARD_OCM_SPECTRUM_POWER_NOTIFY == id) {
            m_ocmScanCallback = (otai_linecard_ocm_spectrum_power_notification_fn)attr_list[idx].value.ptr;
        }

        if(OTAI_LINECARD_ATTR_LINECARD_OTDR_RESULT_NOTIFY == id) {
            m_otdrScanCallback = (otai_linecard_otdr_result_notification_fn)attr_list[idx].value.ptr;
        }

        if(OTAI_APS_ATTR_SWITCH_INFO_NOTIFY == id) {
            m_apsSwitchCallback = (otai_aps_report_switch_info_fn)attr_list[idx].value.ptr;
        }
    }
}

[[noreturn]] void OtaiObjectNotificationSim::sendLinecardNotifications(otai_object_id_t linecard_id)
{
    this_thread::sleep_for(chrono::seconds(10));
    sendAlarmNotification(linecard_id);
    otai_oper_status_t linecardOperStatus = OTAI_OPER_STATUS_ACTIVE;
    while (true)
    {
        otai_oper_status_t operStatus = readOperStatus("otai_linecard_notification_sim.json");
        if(operStatus != linecardOperStatus) {
            sendLinecardStateNotification(linecard_id, operStatus);
            linecardOperStatus = operStatus;
        }

        this_thread::sleep_for(chrono::seconds(5));
    }
}

otai_oper_status_t OtaiObjectNotificationSim::readOperStatus(string filename) 
{
    otai_oper_status_t result = OTAI_OPER_STATUS_ACTIVE;
    string sim_data_file = sim_data_path + filename;

    std::ifstream ifs(sim_data_file);
    if (!ifs.good())
    {
        SWSS_LOG_ERROR("OtaiObjectSimulator failed to read '%s', err: %s", sim_data_file, strerror(errno));
        return result;
    }

    try
    {
        nlohmann::json jsonData = json::parse(ifs);
        result = (otai_oper_status_t)jsonData["linecard_oper_status"];
    }
    catch (const std::exception& e)
    {
        SWSS_LOG_ERROR("OtaiObjectSimulator Failed to parse '%s': %s", sim_data_file, e.what());
    }

    return result;
}

void OtaiObjectNotificationSim::sendAlarmNotification(otai_object_id_t linecard_id)
{
    otai_alarm_type_t alarm_type = OTAI_ALARM_TYPE_HIGH_TEMPERATURE_ALARM;
    otai_alarm_info_t alarm_info;
    alarm_info.resource_oid = linecard_id;
    alarm_info.time_created = (uint64_t)chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch()).count();
    alarm_info.status = OTAI_ALARM_STATUS_ACTIVE;
    alarm_info.severity = OTAI_ALARM_SEVERITY_MAJOR;
    char text[] = "high temperature";
    alarm_info.text.count = static_cast<uint32_t>(strlen(text));
    alarm_info.text.list = (int8_t *)text;

    SWSS_LOG_NOTICE("Sending notification for linecard:%s alarm",
            otai_serialize_object_id(linecard_id).c_str());
    m_alarmCallback(linecard_id, alarm_type, alarm_info); 
}

void OtaiObjectNotificationSim::sendOcmScanNotification(otai_object_id_t linecard_id, otai_object_id_t ocm_id)
{
    this_thread::sleep_for(chrono::seconds(10));

    otai_spectrum_power_t list[96];
    otai_uint64_t freq = 194400000;
    otai_uint64_t freq_step = 12500;
    for (int i = 0; i < 96; i++)
    {
        list[i].lower_frequency = freq;
        list[i].upper_frequency = freq + freq_step;
        list[i].power = -10.0;

        freq += freq_step;
    }

    otai_spectrum_power_list_t ocm_result;
    ocm_result.count = 96;
    ocm_result.list = list; 

    SWSS_LOG_NOTICE("Sending notification for ocm %s scan info", 
            otai_serialize_object_id(ocm_id).c_str());
    m_ocmScanCallback(linecard_id, ocm_id, ocm_result);
}

void OtaiObjectNotificationSim::sendOtdrScanNotification(otai_object_id_t linecard_id, otai_object_id_t otdr_id)
{
    this_thread::sleep_for(chrono::seconds(10));

    otai_otdr_result_t result;
    result.scanning_profile.scan_time = (uint64_t)chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch()).count();
    result.scanning_profile.distance_range = 80;
    result.scanning_profile.pulse_width = 20;
    result.scanning_profile.average_time = 300;
    result.scanning_profile.output_frequency = 125000;

    otai_otdr_event_t events[] = 
    {
        {
            .type = OTAI_OTDR_EVENT_TYPE_START,
            .length = 0.0,
            .loss = -40.0,
            .reflection = 1.0,
            .accumulate_loss = -40.0,
        },
        {
            .type = OTAI_OTDR_EVENT_TYPE_END,
            .length = 80.1,
            .loss = -38.1,
            .reflection = 1.0,
            .accumulate_loss = -38.0,
        }
    };

    result.events.span_distance = 80.1;
    result.events.span_loss = -38.0;
    result.events.events.count = 2;
    result.events.events.list = events;

    result.trace.update_time = result.scanning_profile.scan_time;

    uint8_t data[400000];
    uint8_t count = 0;
    for (uint32_t i = 0; i < sizeof(data)/sizeof(uint8_t); i++)
    {
        data[i] = count++;
    }

    result.trace.data.count = sizeof(data)/sizeof(uint8_t);
    result.trace.data.list = data;

    SWSS_LOG_NOTICE("Sending notification for otdr %s scan info", 
            otai_serialize_object_id(otdr_id).c_str());
    m_otdrScanCallback(linecard_id, otdr_id, result);
}

void OtaiObjectNotificationSim::sendLinecardStateNotification(otai_object_id_t linecard_id, otai_oper_status_t status)
{
    SWSS_LOG_NOTICE("Sending notification for linecard status change:%s",
            otai_serialize_linecard_oper_status(linecard_id, status).c_str());
    m_stateChgCallback(linecard_id, status);  
}

void OtaiObjectNotificationSim::sendApsSwitchNotification(otai_object_id_t aps_id)
{
    this_thread::sleep_for(chrono::seconds(10));

    otai_olp_switch_t switch_info;
    switch_info.num = 1;
    switch_info.type = 1;
    switch_info.interval = 2;
    switch_info.pointers = 81;
    switch_info.channel_id = 1;

    otai_olp_switch_info_t info;
    info.index = 1;
    info.reason = OTAI_OLP_SWITCH_REASON_FORCE_CMD;
    info.operate = OTAI_OLP_SWITCH_OPERATE_PRIMARY_TO_SECONDARY;
    info.time_stamp = (uint64_t)chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch()).count();
    
    otai_olp_switch_power_info_t power_info;
    power_info.common_out = -13.4;
    power_info.primary_in = -13.44;
    power_info.secondary_in = -6.28;
    info.switching = power_info;
    for(int i = 0; i < 40; i++) {
        info.before[i] = power_info;
        info.after[i] = power_info;
    }
    switch_info.info[0] = info;

    SWSS_LOG_NOTICE("Sending notification for aps %s swith info for", 
            otai_serialize_object_id(aps_id).c_str());
    m_apsSwitchCallback(aps_id, switch_info);
}