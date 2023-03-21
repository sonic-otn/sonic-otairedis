#pragma once

#include "Notification.h"

namespace lairedis
{
    class NotificationLinecardAlarm:
        public Notification
    {
        public:

            NotificationLinecardAlarm(
                    _In_ const std::string& serializedNotification);

            virtual ~NotificationLinecardAlarm();

        public:

            virtual lai_object_id_t getLinecardId() const override;

            virtual lai_object_id_t getAnyObjectId() const override;

            virtual void processMetadata(
                    _In_ std::shared_ptr<laimeta::Meta> meta) const override;

            virtual void executeCallback(
                    _In_ const lai_linecard_notifications_t& linecardNotifications) const override;

        private:

            lai_object_id_t m_linecardId;
            lai_alarm_type_t m_alarm_type;
            lai_alarm_info_t m_alarm_info;

    };
}
