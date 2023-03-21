#pragma once

extern "C"{
#include "laimetadata.h"
}

#include "EventPayload.h"

#include "lib/inc/Notification.h"

namespace laivs
{
    class EventPayloadNotification:
        public EventPayload
    {
        public:

            EventPayloadNotification(
                    _In_ std::shared_ptr<lairedis::Notification> ntf,
                    _In_ const lai_linecard_notifications_t& linecardNotifications);

            virtual ~EventPayloadNotification() = default;

        public:

            std::shared_ptr<lairedis::Notification> getNotification() const;

            const lai_linecard_notifications_t& getLinecardNotifications() const;

        private:

            std::shared_ptr<lairedis::Notification> m_ntf;

            lai_linecard_notifications_t m_linecardNotifications;
    };
}
