#include "NotificationFactory.h"
#include "NotificationLinecardStateChange.h"
#include "NotificationOcmNotify.h"
#include "otairediscommon.h"

#include "swss/logger.h"

using namespace otairedis;

std::shared_ptr<Notification> NotificationFactory::deserialize(
        _In_ const std::string& name,
        _In_ const std::string& serializedNotification)
{
    SWSS_LOG_ENTER();

    if (name == OTAI_LINECARD_NOTIFICATION_NAME_LINECARD_STATE_CHANGE)
        return std::make_shared<NotificationLinecardStateChange>(serializedNotification);

    if (name == OTAI_OCM_NOTIFICATION_NAME_SPECTRUM_POWER_NOTIFY)
        return std::make_shared<NotificationOcmNotify>(serializedNotification);

    SWSS_LOG_ERROR("unknown notification: '%s', FIXME", name.c_str());

    return nullptr;
}
