#include "EventPayloadNetLinkMsg.h"

#include "swss/logger.h"

using namespace laivs;

EventPayloadNetLinkMsg::EventPayloadNetLinkMsg(
        _In_ lai_object_id_t linecardId,
        _In_ int nlmsgType,
        _In_ int ifIndex,
        _In_ unsigned int ifFlags,
        _In_ const std::string& ifName):
    m_linecardId(linecardId),
    m_nlmsgType(nlmsgType),
    m_ifIndex(ifIndex),
    m_ifFlags(ifFlags),
    m_ifName(ifName)
{
    SWSS_LOG_ENTER();

    // empty
}

lai_object_id_t EventPayloadNetLinkMsg::getLinecardId() const
{
    SWSS_LOG_ENTER();

    return m_linecardId;
}

int EventPayloadNetLinkMsg::getNlmsgType() const
{
    SWSS_LOG_ENTER();

    return m_nlmsgType;
}

int EventPayloadNetLinkMsg::getIfIndex() const
{
    SWSS_LOG_ENTER();

    return m_ifIndex;
}

unsigned int EventPayloadNetLinkMsg::getIfFlags() const
{
    SWSS_LOG_ENTER();

    return m_ifFlags;
}

const std::string& EventPayloadNetLinkMsg::getIfName() const
{
    SWSS_LOG_ENTER();

    return m_ifName;
}
