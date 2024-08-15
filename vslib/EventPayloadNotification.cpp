#include "EventPayloadNotification.h"

#include "swss/logger.h"

using namespace otaivs;

EventPayloadNotification::EventPayloadNotification(
        _In_ std::shared_ptr<otairedis::Notification> ntf,
        _In_ const otai_linecard_notifications_t& linecardNotifications):
    m_ntf(ntf),
    m_linecardNotifications(linecardNotifications)
{
    SWSS_LOG_ENTER();

    // empty
}

std::shared_ptr<otairedis::Notification> EventPayloadNotification::getNotification() const
{
    SWSS_LOG_ENTER();

    return m_ntf;
}

const otai_linecard_notifications_t& EventPayloadNotification::getLinecardNotifications() const
{
    SWSS_LOG_ENTER();

    return m_linecardNotifications;
}
