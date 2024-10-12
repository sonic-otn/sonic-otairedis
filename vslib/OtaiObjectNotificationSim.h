#pragma once

#include <string>
#include "otaitypes.h"
#include "meta/otai_serialize.h"

using namespace std;

const string sim_data_path = "/usr/include/vslib/otai_sim_data/";

namespace otaivs
{
    class OtaiObjectNotificationSim 
    {
        public:
            void updateNotificationCallback(uint32_t attr_count, const otai_attribute_t *attr_list); 
            void triggerLinecardNtfs(otai_object_id_t linecard_id);
            void triggerOcmScanNtfs(otai_object_id_t linecard_id, otai_object_id_t ocm_id);
            void triggerOtdrScanNtfs(otai_object_id_t linecard_id, otai_object_id_t ocm_id);
            void triggerApsSwitchNtfs(otai_object_id_t aps_id);

        private:    
            void sendLinecardNotifications(otai_object_id_t linecard_id);
            void sendAlarmNotification(otai_object_id_t linecard_id);
            void sendOcmScanNotification(otai_object_id_t linecard_id, otai_object_id_t ocm_id);
            void sendOtdrScanNotification(otai_object_id_t linecard_id, otai_object_id_t otdr_id);
            void sendLinecardStateNotification(otai_object_id_t linecard_id, otai_oper_status_t status);
            void sendApsSwitchNotification(otai_object_id_t aps_id);
            otai_oper_status_t readOperStatus(string filename); 


        private:
            std::thread m_linecardNotifThread;
            std::thread m_ocmNotifThread;
            std::thread m_otdrNotifThread;
            std::thread m_apsNotifThread;  

        private:
            // notification callback function
            otai_linecard_alarm_notification_fn                      m_alarmCallback ;
            otai_linecard_state_change_notification_fn               m_stateChgCallback;
            otai_linecard_ocm_spectrum_power_notification_fn         m_ocmScanCallback;
            otai_linecard_otdr_result_notification_fn                m_otdrScanCallback;
            otai_aps_report_switch_info_fn                           m_apsSwitchCallback;          
    };
}