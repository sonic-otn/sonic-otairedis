#pragma once

extern "C"{
#include "otaimetadata.h"
}

#include "EventPayload.h"

#include "meta/Notification.h"

namespace otaivs
{
    class EventPayloadNotification:
        public EventPayload
    {
        public:

            EventPayloadNotification(
                    _In_ std::shared_ptr<otairedis::Notification> ntf,
                    _In_ const otai_linecard_notifications_t& linecardNotifications);

            virtual ~EventPayloadNotification() = default;

        public:

            std::shared_ptr<otairedis::Notification> getNotification() const;

            const otai_linecard_notifications_t& getLinecardNotifications() const;

        private:

            std::shared_ptr<otairedis::Notification> m_ntf;

            otai_linecard_notifications_t m_linecardNotifications;
    };
}
