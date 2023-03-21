#pragma once

extern "C"{
#include "laimetadata.h"
}

#include "swss/logger.h"

#include <functional>
#include <vector>

typedef struct _lai_notifications_t {
    lai_linecard_alarm_notification_fn on_linecard_alarm;
    lai_linecard_state_change_notification_fn on_linecard_state_change;
    lai_aps_report_switch_info_fn on_aps_report_switch_info;
} lai_notifications_t;

namespace syncd
{
    class LinecardNotifications
    {
        private:

            class SlotBase
            {
                public:

                    SlotBase(
                            _In_ lai_notifications_t sn);

                    virtual ~SlotBase();

                public:

                    void setHandler(
                            _In_ LinecardNotifications* handler);

                    LinecardNotifications* getHandler() const;

                    const lai_notifications_t& getLinecardNotifications() const;

                protected:

                    static void onLinecardStateChange(
                            _In_ int context,
                            _In_ lai_object_id_t linecard_id,
                            _In_ lai_oper_status_t linecard_oper_status);

                    static void onLinecardAlarm(
                            _In_ int context,
                            _In_ lai_object_id_t linecard_id,
                            _In_ lai_alarm_type_t alarm_type,
                            _In_ lai_alarm_info_t alarm_info);

                    static void onApsReportSwitchInfo(
                            _In_ int context,
                            _In_ lai_object_id_t aps_id,
                            _In_ lai_olp_switch_t switch_info);

                protected:

                    LinecardNotifications* m_handler;

                    lai_notifications_t m_sn;
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
                            }) { }

                virtual ~Slot() {}

            private:

                static void onLinecardStateChange(
                        _In_ lai_object_id_t linecard_id,
                        _In_ lai_oper_status_t linecard_oper_status)
                {
                    SWSS_LOG_ENTER();

                    return SlotBase::onLinecardStateChange(context, linecard_id, linecard_oper_status);
                }

                static void onLinecardAlarm(
                        _In_ lai_object_id_t linecard_id,
                        _In_ lai_alarm_type_t alarm_type,
                        _In_ lai_alarm_info_t alarm_info)
                {
                    SWSS_LOG_ENTER();

                    return SlotBase::onLinecardAlarm(context, linecard_id, alarm_type, alarm_info);
                }

                static void onApsReportSwitchInfo(
                        _In_ lai_object_id_t aps_id,
                        _In_ lai_olp_switch_t switch_info)
                {
                    SWSS_LOG_ENTER();

                    return SlotBase::onApsReportSwitchInfo(context, aps_id, switch_info);
                }

        };

            static std::vector<LinecardNotifications::SlotBase*> m_slots;

        public:

            LinecardNotifications();

            virtual ~LinecardNotifications();

        public:

            const lai_notifications_t& getLinecardNotifications() const;

        public: // wrapped methods

            std::function<void(lai_object_id_t linecard_id, lai_oper_status_t)>                  onLinecardStateChange;
            std::function<void(lai_object_id_t linecard_id, lai_alarm_type_t, lai_alarm_info_t)> onLinecardAlarm;
            std::function<void(lai_object_id_t aps_id, lai_olp_switch_t)> onApsReportSwitchInfo;

        private:

            SlotBase*m_slot;
    };
}
