#pragma once

extern "C"{
#include "otaimetadata.h"
}

#include "swss/logger.h"

#include <functional>
#include <vector>

typedef struct _otai_notifications_t {
    otai_linecard_alarm_notification_fn on_linecard_alarm;
    otai_linecard_state_change_notification_fn on_linecard_state_change;
    otai_aps_report_switch_info_fn on_aps_report_switch_info;
    otai_linecard_ocm_spectrum_power_notification_fn on_linecard_ocm_spectrum_power;
    otai_linecard_otdr_result_notification_fn on_linecard_otdr_result;
} otai_notifications_t;

namespace syncd
{
    class LinecardNotifications
    {
        private:

            class SlotBase
            {
                public:

                    SlotBase(
                            _In_ otai_notifications_t sn);

                    virtual ~SlotBase();

                public:

                    void setHandler(
                            _In_ LinecardNotifications* handler);

                    LinecardNotifications* getHandler() const;

                    const otai_notifications_t& getLinecardNotifications() const;

                protected:

                    static void onLinecardStateChange(
                            _In_ int context,
                            _In_ otai_object_id_t linecard_id,
                            _In_ otai_oper_status_t linecard_oper_status);

                    static void onLinecardAlarm(
                            _In_ int context,
                            _In_ otai_object_id_t linecard_id,
                            _In_ otai_alarm_type_t alarm_type,
                            _In_ otai_alarm_info_t alarm_info);

                    static void onApsReportSwitchInfo(
                            _In_ int context,
                            _In_ otai_object_id_t aps_id,
                            _In_ otai_olp_switch_t switch_info);

                    static void onOcmReportSpectrumPower(
                            _In_ int context,
                            _In_ otai_object_id_t linecard_id,
                            _In_ otai_object_id_t ocm_id,
                            _In_ otai_spectrum_power_list_t ocm_result);
                    
                    static void onOtdrReportResult(
                            _In_ int context,
                            _In_ otai_object_id_t linecard_id,
                            _In_ otai_object_id_t otdr_id,
                            _In_ otai_otdr_result_t otdr_result);

                protected:

                    LinecardNotifications* m_handler;

                    otai_notifications_t m_sn;
            };

            template<size_t context>
                class Slot:
                    public SlotBase
        {
            public:

                Slot():
                    SlotBase({
                            .on_linecard_alarm = &Slot<context>::onLinecardAlarm,
                            .on_linecard_state_change = &Slot<context>::onLinecardStateChange,
                            .on_aps_report_switch_info = &Slot<context>::onApsReportSwitchInfo,
                            .on_linecard_ocm_spectrum_power = &Slot<context>::onOcmReportSpectrumPower,
                            .on_linecard_otdr_result = &Slot<context>::onOtdrReportResult,
                            }) { }

                virtual ~Slot() {}

            private:

                static void onLinecardStateChange(
                        _In_ otai_object_id_t linecard_id,
                        _In_ otai_oper_status_t linecard_oper_status)
                {
                    SWSS_LOG_ENTER();

                    return SlotBase::onLinecardStateChange(context, linecard_id, linecard_oper_status);
                }

                static void onLinecardAlarm(
                        _In_ otai_object_id_t linecard_id,
                        _In_ otai_alarm_type_t alarm_type,
                        _In_ otai_alarm_info_t alarm_info)
                {
                    SWSS_LOG_ENTER();

                    return SlotBase::onLinecardAlarm(context, linecard_id, alarm_type, alarm_info);
                }

                static void onApsReportSwitchInfo(
                        _In_ otai_object_id_t aps_id,
                        _In_ otai_olp_switch_t switch_info)
                {
                    SWSS_LOG_ENTER();

                    return SlotBase::onApsReportSwitchInfo(context, aps_id, switch_info);
                }

                static void onOcmReportSpectrumPower(
                        _In_ otai_object_id_t linecard_id,
                        _In_ otai_object_id_t ocm_id,
                        _In_ otai_spectrum_power_list_t ocm_result)
                {
                    SWSS_LOG_ENTER();

                    return SlotBase::onOcmReportSpectrumPower(context, linecard_id, ocm_id, ocm_result);
                }

                static void onOtdrReportResult(
                        _In_ otai_object_id_t linecard_id,
                        _In_ otai_object_id_t otdr_id,
                        _In_ otai_otdr_result_t otdr_result)
                {
                    SWSS_LOG_ENTER();

                    return SlotBase::onOtdrReportResult(context, linecard_id, otdr_id, otdr_result);
                }

        };

            static std::vector<LinecardNotifications::SlotBase*> m_slots;

        public:

            LinecardNotifications();

            virtual ~LinecardNotifications();

        public:

            const otai_notifications_t& getLinecardNotifications() const;

        public: // wrapped methods

            std::function<void(otai_object_id_t linecard_id, otai_oper_status_t)>                  onLinecardStateChange;
            std::function<void(otai_object_id_t linecard_id, otai_alarm_type_t, otai_alarm_info_t)> onLinecardAlarm;
            std::function<void(otai_object_id_t aps_id, otai_olp_switch_t)>                        onApsReportSwitchInfo;
            std::function<void(otai_object_id_t linecard_id, otai_object_id_t ocm_id, otai_spectrum_power_list_t)>               onOcmReportSpectrumPower;
            std::function<void(otai_object_id_t linecard_id, otai_object_id_t otdr_id, otai_otdr_result_t)>                      onOtdrReportResult;

        private:

            SlotBase*m_slot;
    };
}
