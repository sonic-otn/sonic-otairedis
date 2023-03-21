#include "Context.h"

#include "swss/logger.h"

using namespace lairedis;
using namespace std::placeholders;

Context::Context(
        _In_ std::shared_ptr<ContextConfig> contextConfig,
        _In_ std::shared_ptr<Recorder> recorder,
        _In_ std::function<lai_linecard_notifications_t(std::shared_ptr<Notification>, Context*)> notificationCallback):
    m_contextConfig(contextConfig),
    m_recorder(recorder),
    m_notificationCallback(notificationCallback)
{
    SWSS_LOG_ENTER();

    // will create notification thread
    m_redisLai = std::make_shared<RedisRemoteLaiInterface>(
            m_contextConfig,
            std::bind(&Context::handle_notification, this, _1),
            m_recorder);

    m_meta = std::make_shared<laimeta::Meta>(m_redisLai);

    m_redisLai->setMeta(m_meta);
}

Context::~Context()
{
    SWSS_LOG_ENTER();

    m_redisLai->uninitialize(); // will stop threads

    m_redisLai = nullptr;

    m_meta = nullptr;
}

lai_linecard_notifications_t Context::handle_notification(
        _In_ std::shared_ptr<Notification> notification)
{
    SWSS_LOG_ENTER();

    return m_notificationCallback(notification, this);
}

