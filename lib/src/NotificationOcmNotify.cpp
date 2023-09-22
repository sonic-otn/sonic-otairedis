#include "NotificationOcmNotify.h"

#include "swss/logger.h"

#include "meta/otai_serialize.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include "swss/json.hpp"
#pragma GCC diagnostic pop

using json = nlohmann::json;

using namespace otairedis;

NotificationOcmNotify::NotificationOcmNotify(
        _In_ const std::string& serializedNotification):
    Notification(
            OTAI_LINECARD_NOTIFICATION_TYPE_LINECARD_OCM_SPECTRUM_POWER,
            serializedNotification)
{
    SWSS_LOG_ENTER();

    json j = json::parse(serializedNotification);

    otai_deserialize_object_id(j["linecard_id"], m_linecardId);
    otai_deserialize_object_id(j["ocm_id"], m_ocmId);

    m_spectrumPowerList.count = 0;
    m_spectrumPowerList.list = nullptr;
}

otai_object_id_t NotificationOcmNotify::getLinecardId() const
{
    SWSS_LOG_ENTER();

    return m_linecardId;
}

otai_object_id_t NotificationOcmNotify::getAnyObjectId() const
{
    SWSS_LOG_ENTER();

    return m_ocmId;
}

void NotificationOcmNotify::processMetadata(
        _In_ std::shared_ptr<otaimeta::Meta> meta) const
{
    SWSS_LOG_ENTER();
}

void NotificationOcmNotify::executeCallback(
        _In_ const otai_linecard_notifications_t& linecardNotifications) const
{
    SWSS_LOG_ENTER();

    if (linecardNotifications.on_linecard_ocm_spectrum_power)
    {
        linecardNotifications.on_linecard_ocm_spectrum_power(m_linecardId, m_ocmId, m_spectrumPowerList);
    }
}
