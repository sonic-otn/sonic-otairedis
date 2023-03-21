#pragma once

extern "C" {
#include "laimetadata.h"
}

#include "meta/Meta.h"

#include <string>
#include <memory>

namespace lairedis
{
    class Notification
    {
        public:

            Notification(
                    _In_ lai_linecard_notification_type_t linecardNotificationType,
                    _In_ const std::string& serializedNotification);

            virtual ~Notification() = default;

        public:

            /**
             * @brief Return linecard ID for notification.
             *
             * If notification contains linecard id field, then returns value of
             * that field. If notification contains multiple linecard id fields,
             * first one is returned.
             *
             * If notification don't contain linecard id field, return value is
             * LAI_NULL_OBJECT_ID.
             */
            virtual lai_object_id_t getLinecardId() const = 0;

            /**
             * @brief Get any object id.
             *
             * Some notifications may not contain any linecard id field, like for
             * example queue_deadlock_notification, then any object id defined
             * on that notification will be returned.
             *
             * If notification defines linecard if object and is not NULL then it
             * should be returned.
             *
             * This object id will be used to determine linecard id using
             * lai_linecard_id_query() API.
             *
             * If no other object besides linecard id is defined, then this
             * function returns linecard id.
             */
            virtual lai_object_id_t getAnyObjectId() const = 0;

            /**
             * @brief Process metadata for notification.
             *
             * Metadata database must be aware of each notifications when they
             * arrive, this will pass notification data to notification
             * function.
             *
             * This function must be executed under lairedis API mutex.
             */
            virtual void processMetadata(
                    std::shared_ptr<laimeta::Meta> meta) const = 0;

            /**
             * @brief Execute callback notification.
             *
             * Execute callback notification on right notification pointer passing
             * deserialized data.
             */
            virtual void executeCallback(
                    _In_ const lai_linecard_notifications_t& linecardNotifications) const = 0;

        public:

            /**
             * @brief Get notification type.
             */
            lai_linecard_notification_type_t getNotificationType() const;

            /**
             * @brief Get serialized notification.
             *
             * Contains all notification data without notification name.
             */
            const std::string& getSerializedNotification() const;

        private:

            const lai_linecard_notification_type_t m_linecardNotificationType;

            const std::string m_serializedNotification;
    };
}
