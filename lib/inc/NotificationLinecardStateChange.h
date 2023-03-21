#pragma once

#include "Notification.h"

namespace lairedis
{
    class NotificationLinecardStateChange:
        public Notification
    {
        public:

            NotificationLinecardStateChange(
                    _In_ const std::string& serializedNotification);

            virtual ~NotificationLinecardStateChange() = default;

        public:

            virtual lai_object_id_t getLinecardId() const override;

            virtual lai_object_id_t getAnyObjectId() const override;

            virtual void processMetadata(
                    _In_ std::shared_ptr<laimeta::Meta> meta) const override;

            virtual void executeCallback(
                    _In_ const lai_linecard_notifications_t& linecardNotifications) const override;

        private:

            lai_object_id_t m_linecardId;

            lai_oper_status_t m_linecardOperStatus;
    };
}
