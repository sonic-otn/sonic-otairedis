#include "EventPayloadNotification.h"

#include "swss/logger.h"

using namespace laivs;

EventPayloadNotification::EventPayloadNotification(
        _In_ std::shared_ptr<lairedis::Notification> ntf,
        _In_ const lai_linecard_notifications_t& linecardNotifications):
    m_ntf(ntf),
    m_linecardNotifications(linecardNotifications)
{
    SWSS_LOG_ENTER();

    // empty
}

std::shared_ptr<lairedis::Notification> EventPayloadNotification::getNotification() const
{
    SWSS_LOG_ENTER();

    return m_ntf;
}

const lai_linecard_notifications_t& EventPayloadNotification::getLinecardNotifications() const
{
    SWSS_LOG_ENTER();

    return m_linecardNotifications;
}
