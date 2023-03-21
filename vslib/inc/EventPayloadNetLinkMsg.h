#pragma once

extern "C" {
#include "lai.h"
}

#include "EventPayload.h"

#include "swss/sal.h"

#include <string>

namespace laivs
{
    class EventPayloadNetLinkMsg:
        public EventPayload
    {
        public:

            EventPayloadNetLinkMsg(
                    _In_ lai_object_id_t linecardId,
                    _In_ int nlmsgType,
                    _In_ int ifIndex,
                    _In_ unsigned int ifFlags,
                    _In_ const std::string& ifName);

            virtual ~EventPayloadNetLinkMsg() = default;

        public:

            lai_object_id_t getLinecardId() const;

            int getNlmsgType() const;

            int getIfIndex() const;

            unsigned int getIfFlags() const;

            const std::string& getIfName() const;

        private:

            lai_object_id_t m_linecardId;

            int m_nlmsgType;

            int m_ifIndex;

            unsigned int m_ifFlags;

            std::string m_ifName;
    };
}

