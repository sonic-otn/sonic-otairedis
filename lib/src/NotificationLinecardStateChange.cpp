#include "NotificationLinecardStateChange.h"

#include "swss/logger.h"

#include "meta/lai_serialize.h"

namespace lairedis
{

NotificationLinecardStateChange::NotificationLinecardStateChange(
        _In_ const std::string& serializedNotification):
    Notification(
            LAI_LINECARD_NOTIFICATION_TYPE_LINECARD_STATE_CHANGE,
            serializedNotification)
{
    SWSS_LOG_ENTER();

    lai_deserialize_linecard_oper_status(
            serializedNotification,
            m_linecardId,
            m_linecardOperStatus);
}

lai_object_id_t NotificationLinecardStateChange::getLinecardId() const
{
    SWSS_LOG_ENTER();

    return m_linecardId;
}

lai_object_id_t NotificationLinecardStateChange::getAnyObjectId() const
{
    SWSS_LOG_ENTER();

    return m_linecardId;
}

void NotificationLinecardStateChange::processMetadata(
        _In_ std::shared_ptr<laimeta::Meta> meta) const
{
    SWSS_LOG_ENTER();

    meta->meta_lai_on_linecard_state_change(m_linecardId, m_linecardOperStatus);
}

void NotificationLinecardStateChange::executeCallback(
        _In_ const lai_linecard_notifications_t& linecardNotifications) const
{
    SWSS_LOG_ENTER();

    if (linecardNotifications.on_linecard_state_change)
    {
        linecardNotifications.on_linecard_state_change(m_linecardId, m_linecardOperStatus);
    }
}


}
