#pragma once

extern "C" {
#include "laimetadata.h"
}

namespace laivs
{
    class Linecard
    {
        public:

            Linecard(
                    _In_ lai_object_id_t linecardId);

            Linecard(
                    _In_ lai_object_id_t linecardId,
                    _In_ uint32_t attrCount,
                    _In_ const lai_attribute_t *attrList);

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
                    _In_ const lai_attribute_t *attrList);

            const lai_linecard_notifications_t& getLinecardNotifications() const;

            lai_object_id_t getLinecardId() const;

        private:

            lai_object_id_t m_linecardId;

            /**
             * @brief Notifications pointers holder.
             *
             * Each linecard instance can have it's own notifications defined.
             */
            lai_linecard_notifications_t m_linecardNotifications;
    };
}
