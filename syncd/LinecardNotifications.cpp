#include "LinecardNotifications.h"

#include "swss/logger.h"

using namespace syncd;

LinecardNotifications::SlotBase::SlotBase(
        _In_ lai_notifications_t sn):
    m_handler(nullptr),
    m_sn(sn)
{
    SWSS_LOG_ENTER();

    // empty
}

LinecardNotifications::SlotBase::~SlotBase()
{
    SWSS_LOG_ENTER();

    // empty
}

void LinecardNotifications::SlotBase::setHandler(
        _In_ LinecardNotifications* handler)
{
    SWSS_LOG_ENTER();

    m_handler = handler;
}

LinecardNotifications* LinecardNotifications::SlotBase::getHandler() const
{
    SWSS_LOG_ENTER();

    return m_handler;
}

void LinecardNotifications::SlotBase::onLinecardStateChange(
        _In_ int context,
        _In_ lai_object_id_t linecard_id,
        _In_ lai_oper_status_t linecard_oper_status)
{
    SWSS_LOG_ENTER();

    return m_slots.at(context)->m_handler->onLinecardStateChange(linecard_id, linecard_oper_status);
}

void LinecardNotifications::SlotBase::onLinecardAlarm(
        _In_ int context,
        _In_ lai_object_id_t linecard_id,
        _In_ lai_alarm_type_t alarm_type,
        _In_ lai_alarm_info_t alarm_info)
{
    SWSS_LOG_ENTER();

    return m_slots.at(context)->m_handler->onLinecardAlarm(linecard_id, alarm_type, alarm_info);
}

void LinecardNotifications::SlotBase::onApsReportSwitchInfo(
        _In_ int context,
        _In_ lai_object_id_t aps_id,
        _In_ lai_olp_switch_t switch_info)
{
    SWSS_LOG_ENTER();

    return m_slots.at(context)->m_handler->onApsReportSwitchInfo(aps_id, switch_info);
}

void LinecardNotifications::SlotBase::onOcmReportSpectrumPower(
        _In_ int context,
        _In_ lai_object_id_t linecard_id,
        _In_ lai_object_id_t ocm_id,
        _In_ lai_spectrum_power_list_t ocm_result)
{
    SWSS_LOG_ENTER();

    return m_slots.at(context)->m_handler->onOcmReportSpectrumPower(linecard_id, ocm_id, ocm_result);
}

void LinecardNotifications::SlotBase::onOtdrReportResult(
        _In_ int context,
        _In_ lai_object_id_t linecard_id,
        _In_ lai_object_id_t otdr_id,
        _In_ lai_otdr_result_t otdr_result)
{
    SWSS_LOG_ENTER();

    return m_slots.at(context)->m_handler->onOtdrReportResult(linecard_id, otdr_id, otdr_result);
}

const lai_notifications_t& LinecardNotifications::SlotBase::getLinecardNotifications() const
{
    SWSS_LOG_ENTER();

    return m_sn;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winline"
#if 0
template<class B, template<size_t> class D, size_t... i>
static constexpr auto declare_static(std::index_sequence<i...>)
{
    SWSS_LOG_ENTER();
    return std::array<B*, sizeof...(i)>{{new D<i>()...}};
}

template<class B, template<size_t> class D, size_t size>
static constexpr auto declare_static()
{
    SWSS_LOG_ENTER();
    auto arr = declare_static<B,D>(std::make_index_sequence<size>{});
    return std::vector<B*>{arr.begin(), arr.end()};
}

std::vector<LinecardNotifications::SlotBase*> LinecardNotifications::m_slots =
    declare_static<LinecardNotifications::SlotBase, LinecardNotifications::Slot, 0x10>();
#endif
std::vector<LinecardNotifications::SlotBase*> LinecardNotifications::m_slots = {
    new LinecardNotifications::Slot<0x00>(),
    new LinecardNotifications::Slot<0x01>(),
    new LinecardNotifications::Slot<0x02>(),
    new LinecardNotifications::Slot<0x03>(),
    new LinecardNotifications::Slot<0x04>(),
    new LinecardNotifications::Slot<0x05>(),
    new LinecardNotifications::Slot<0x06>(),
    new LinecardNotifications::Slot<0x07>(),
    new LinecardNotifications::Slot<0x08>(),
    new LinecardNotifications::Slot<0x09>(),
    new LinecardNotifications::Slot<0x0A>(),
    new LinecardNotifications::Slot<0x0B>(),
    new LinecardNotifications::Slot<0x0C>(),
    new LinecardNotifications::Slot<0x0D>(),
    new LinecardNotifications::Slot<0x0E>(),
    new LinecardNotifications::Slot<0x0F>(),
};
#pragma GCC diagnostic pop

LinecardNotifications::LinecardNotifications()
{
    SWSS_LOG_ENTER();

    for (auto& slot: m_slots)
    {
        if (slot->getHandler() == nullptr)
        {
            m_slot = slot;

            m_slot->setHandler(this);

            return;
        }
    }

    SWSS_LOG_THROW("no more available slots, max slots: %zu", m_slots.size());
}

LinecardNotifications::~LinecardNotifications()
{
    SWSS_LOG_ENTER();

    m_slot->setHandler(nullptr);
}

const lai_notifications_t& LinecardNotifications::getLinecardNotifications() const
{
    SWSS_LOG_ENTER();

    return m_slot->getLinecardNotifications();
}
