#pragma once

#include "RedisRemoteLaiInterface.h"
#include "Notification.h"
#include "ContextConfig.h"

#include "meta/Meta.h"

namespace lairedis
{
    class Context
    {
        public:

            Context(
                    _In_ std::shared_ptr<ContextConfig> contextConfig,
                    _In_ std::shared_ptr<Recorder> recorder,
                    _In_ std::function<lai_linecard_notifications_t(std::shared_ptr<Notification>, Context*)> notificationCallback);

            virtual ~Context();

        private:

            lai_linecard_notifications_t handle_notification(
                    _In_ std::shared_ptr<Notification> notification);

        private:

            std::shared_ptr<ContextConfig> m_contextConfig;

            std::shared_ptr<Recorder> m_recorder;

        public:

            std::shared_ptr<laimeta::Meta> m_meta;

            std::shared_ptr<RedisRemoteLaiInterface> m_redisLai;

            std::function<lai_linecard_notifications_t(std::shared_ptr<Notification>, Context*)> m_notificationCallback;
    };
}
