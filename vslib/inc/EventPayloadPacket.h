#pragma once

extern "C" {
#include "lai.h"
}

#include "EventPayload.h"
#include "Buffer.h"

#include <string>

namespace laivs
{
    class EventPayloadPacket:
        public EventPayload
    {
        public:

            EventPayloadPacket(
                    _In_ lai_object_id_t port,
                    _In_ int ifIndex,
                    _In_ const std::string& ifName,
                    _In_ const Buffer& buffer);

            virtual ~EventPayloadPacket() = default;

        public:

            lai_object_id_t getPort() const;

            int getIfIndex() const;

            const std::string& getIfName() const;

            const Buffer& getBuffer() const;

        private:

            lai_object_id_t m_port;

            int m_ifIndex;

            std::string m_ifName;

            Buffer m_buffer;
    };
}
