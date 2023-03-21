#include "NotificationLinecardAlarm.h"

#include "swss/logger.h"

#include "meta/lai_serialize.h"

namespace lairedis
{

NotificationLinecardAlarm::NotificationLinecardAlarm(
        _In_ const std::string& serializedNotification):
    Notification(
            LAI_LINECARD_NOTIFICATION_TYPE_LINECARD_ALARM,
            serializedNotification)
{
    SWSS_LOG_ENTER();
	
	lai_deserialize_linecard_alarm(
		serializedNotification,
		m_linecardId,
		m_alarm_type,
		m_alarm_info);
}

NotificationLinecardAlarm::~NotificationLinecardAlarm()
{
	SWSS_LOG_ENTER();
	
	lai_serialize_linecard_alarm(
		m_linecardId,
		m_alarm_type,
		m_alarm_info);
	
}


lai_object_id_t NotificationLinecardAlarm::getLinecardId() const
{
    SWSS_LOG_ENTER();

    return m_linecardId;
}

lai_object_id_t NotificationLinecardAlarm::getAnyObjectId() const
{
    SWSS_LOG_ENTER();

    return m_linecardId;
}

void NotificationLinecardAlarm::processMetadata(
        _In_ std::shared_ptr<laimeta::Meta> meta) const
{
    SWSS_LOG_ENTER();

    //meta->meta_lai_on_linecard_state_change(m_linecardId, m_linecardOperStatus);
}

void NotificationLinecardAlarm::executeCallback(
        _In_ const lai_linecard_notifications_t& linecardNotifications) const
{
    SWSS_LOG_ENTER();

    if (linecardNotifications.on_linecard_alarm)
    {
        linecardNotifications.on_linecard_alarm(m_linecardId, m_alarm_type,m_alarm_info);
    }
}


}
