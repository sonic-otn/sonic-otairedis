#pragma once

#include "Notification.h"

namespace otairedis
{
    class NotificationOcmNotify:
        public Notification
    {
        public:

            NotificationOcmNotify(
                    _In_ const std::string& serializedNotification);
    
            virtual ~NotificationOcmNotify() = default;

        public:

            virtual otai_object_id_t getLinecardId() const override;

            virtual otai_object_id_t getAnyObjectId() const override;

            virtual void processMetadata(
                    _In_ std::shared_ptr<otaimeta::Meta> meta) const override;

            virtual void executeCallback(
                    _In_ const otai_linecard_notifications_t& linecardNotifications) const override;

        private:

            otai_object_id_t m_linecardId;

            otai_object_id_t m_ocmId;

            otai_spectrum_power_list_t m_spectrumPowerList;
    };
}
