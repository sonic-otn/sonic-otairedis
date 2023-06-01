#include "Linecard.h"

#include "swss/logger.h"

#include <cstring>

using namespace laivs;

Linecard::Linecard(
        _In_ lai_object_id_t linecardId):
    m_linecardId(linecardId)
{
    SWSS_LOG_ENTER();

    if (linecardId == LAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("linecard id can't be NULL");
    }

    clearNotificationsPointers();
}

Linecard::Linecard(
        _In_ lai_object_id_t linecardId,
        _In_ uint32_t attrCount,
        _In_ const lai_attribute_t *attrList):
    Linecard(linecardId)
{
    SWSS_LOG_ENTER();

    updateNotifications(attrCount, attrList);
}

void Linecard::clearNotificationsPointers()
{
    SWSS_LOG_ENTER();

    memset(&m_linecardNotifications, 0, sizeof(m_linecardNotifications));
}

lai_object_id_t Linecard::getLinecardId() const
{
    SWSS_LOG_ENTER();

    return m_linecardId;
}

void Linecard::updateNotifications(
        _In_ uint32_t attrCount,
        _In_ const lai_attribute_t *attrList)
{
    SWSS_LOG_ENTER();

    /*
     * This function should only be called on CREATE/SET
     * api when object is LINECARD.
     */

    for (uint32_t index = 0; index < attrCount; ++index)
    {
        auto &attr = attrList[index];

        auto meta = lai_metadata_get_attr_metadata(LAI_OBJECT_TYPE_LINECARD, attr.id);

        if (meta == NULL)
            SWSS_LOG_THROW("failed to find metadata for linecard attr %d", attr.id);

        if (meta->attrvaluetype != LAI_ATTR_VALUE_TYPE_POINTER)
            continue;

        switch (attr.id)
        {
            case LAI_LINECARD_ATTR_LINECARD_STATE_CHANGE_NOTIFY:
                m_linecardNotifications.on_linecard_state_change = 
                    (lai_linecard_state_change_notification_fn)attr.value.ptr;
                break;

            case LAI_LINECARD_ATTR_LINECARD_ALARM_NOTIFY:
                m_linecardNotifications.on_linecard_alarm =
                    (lai_linecard_alarm_notification_fn)attr.value.ptr;
                break;

            case LAI_LINECARD_ATTR_LINECARD_OCM_SPECTRUM_POWER_NOTIFY:
                m_linecardNotifications.on_linecard_ocm_spectrum_power =
                    (lai_linecard_ocm_spectrum_power_notification_fn)attr.value.ptr;
                break;

            case LAI_LINECARD_ATTR_LINECARD_OTDR_RESULT_NOTIFY:
                m_linecardNotifications.on_linecard_otdr_result =
                    (lai_linecard_otdr_result_notification_fn)attr.value.ptr;
                break;

            default:
                SWSS_LOG_ERROR("pointer for %s is not handled, FIXME!", meta->attridname);
                break;
        }
    }
}

const lai_linecard_notifications_t& Linecard::getLinecardNotifications() const
{
    SWSS_LOG_ENTER();

    return m_linecardNotifications;
}

