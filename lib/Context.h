#pragma once

#include "RedisRemoteOtaiInterface.h"
#include "ContextConfig.h"

#include "meta/Notification.h"
#include "meta/Meta.h"

namespace otairedis
{
    class Context
    {
        public:

            Context(
                    _In_ std::shared_ptr<ContextConfig> contextConfig,
                    _In_ std::function<otai_linecard_notifications_t(std::shared_ptr<Notification>, Context*)> notificationCallback);

            virtual ~Context();

        private:

            otai_linecard_notifications_t handle_notification(
                    _In_ std::shared_ptr<Notification> notification);

        private:

            std::shared_ptr<ContextConfig> m_contextConfig;

        public:

            std::shared_ptr<otaimeta::Meta> m_meta;

            std::shared_ptr<RedisRemoteOtaiInterface> m_redisOtai;

            std::function<otai_linecard_notifications_t(std::shared_ptr<Notification>, Context*)> m_notificationCallback;
    };
}
