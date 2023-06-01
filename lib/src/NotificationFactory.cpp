#include "NotificationFactory.h"
#include "NotificationLinecardStateChange.h"
#include "NotificationOcmNotify.h"
#include "lairediscommon.h"

#include "swss/logger.h"

using namespace lairedis;

std::shared_ptr<Notification> NotificationFactory::deserialize(
        _In_ const std::string& name,
        _In_ const std::string& serializedNotification)
{
    SWSS_LOG_ENTER();

    if (name == LAI_LINECARD_NOTIFICATION_NAME_LINECARD_STATE_CHANGE)
        return std::make_shared<NotificationLinecardStateChange>(serializedNotification);

    if (name == LAI_OCM_NOTIFICATION_NAME_SPECTRUM_POWER_NOTIFY)
        return std::make_shared<NotificationOcmNotify>(serializedNotification);

    SWSS_LOG_ERROR("unknown notification: '%s', FIXME", name.c_str());

    return nullptr;
}
