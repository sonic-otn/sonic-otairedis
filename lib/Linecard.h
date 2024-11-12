#pragma once

extern "C" {
#include "otaimetadata.h"
}

#include <string>

namespace otairedis
{
    class Linecard
    {
        public:

            Linecard(
                    _In_ otai_object_id_t linecardId);

            Linecard(
                    _In_ otai_object_id_t linecardId,
                    _In_ uint32_t attrCount,
                    _In_ const otai_attribute_t *attrList);

            virtual ~Linecard() = default;

        public:

            void clearNotificationsPointers();

            /**
             * @brief Update linecard notifications from attribute list.
             *
             * A list of attributes which was passed to create linecard API.
             */
            void updateNotifications(
                    _In_ uint32_t attrCount,
                    _In_ const otai_attribute_t *attrList);

            const otai_linecard_notifications_t& getLinecardNotifications() const;

            otai_object_id_t getLinecardId() const;

        private:

            otai_object_id_t m_linecardId;

            /**
             * @brief Notifications pointers holder.
             *
             * Each linecard instance can have it's own notifications defined.
             */
            otai_linecard_notifications_t m_linecardNotifications;
    };
}
