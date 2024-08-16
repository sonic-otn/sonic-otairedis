#include "Notification.h"

#include "swss/logger.h"

using namespace otairedis;

Notification::Notification(
        _In_ otai_linecard_notification_type_t linecardNotificationType,
        _In_ const std::string& serializedNotification):
    m_linecardNotificationType(linecardNotificationType),
    m_serializedNotification(serializedNotification)
{
    SWSS_LOG_ENTER();

    // empty
}

otai_linecard_notification_type_t Notification::getNotificationType() const
{
    SWSS_LOG_ENTER();

    return m_linecardNotificationType;
}

const std::string& Notification::getSerializedNotification() const
{
    SWSS_LOG_ENTER();

    return m_serializedNotification;
}

