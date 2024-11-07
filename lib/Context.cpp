#include "Context.h"

#include "swss/logger.h"

using namespace otairedis;
using namespace std::placeholders;

Context::Context(
        _In_ std::shared_ptr<ContextConfig> contextConfig,
        _In_ std::function<otai_linecard_notifications_t(std::shared_ptr<Notification>, Context*)> notificationCallback):
    m_contextConfig(contextConfig),
    m_notificationCallback(notificationCallback)
{
    SWSS_LOG_ENTER();

    // will create notification thread
    m_redisOtai = std::make_shared<RedisRemoteOtaiInterface>(
            m_contextConfig,
            std::bind(&Context::handle_notification, this, _1));

    m_meta = std::make_shared<otaimeta::Meta>(m_redisOtai);

    m_redisOtai->setMeta(m_meta);
}

Context::~Context()
{
    SWSS_LOG_ENTER();

    m_redisOtai->uninitialize(); // will stop threads

    m_redisOtai = nullptr;

    m_meta = nullptr;
}

otai_linecard_notifications_t Context::handle_notification(
        _In_ std::shared_ptr<Notification> notification)
{
    SWSS_LOG_ENTER();

    return m_notificationCallback(notification, this);
}

