#pragma once

#include "Notification.h"

namespace lairedis
{
    class NotificationOcmNotify:
        public Notification
    {
        public:

            NotificationOcmNotify(
                    _In_ const std::string& serializedNotification);
    
            virtual ~NotificationOcmNotify() = default;

        public:

            virtual lai_object_id_t getLinecardId() const override;

            virtual lai_object_id_t getAnyObjectId() const override;

            virtual void processMetadata(
                    _In_ std::shared_ptr<laimeta::Meta> meta) const override;

            virtual void executeCallback(
                    _In_ const lai_linecard_notifications_t& linecardNotifications) const override;

        private:

            lai_object_id_t m_linecardId;

            lai_object_id_t m_ocmId;

            lai_spectrum_power_list_t m_spectrumPowerList;
    };
}
