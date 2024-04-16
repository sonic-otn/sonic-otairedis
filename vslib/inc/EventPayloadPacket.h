#pragma once

extern "C" {
#include "otai.h"
}

#include "EventPayload.h"
#include "Buffer.h"

#include <string>

namespace otaivs
{
    class EventPayloadPacket:
        public EventPayload
    {
        public:

            EventPayloadPacket(
                    _In_ otai_object_id_t port,
                    _In_ int ifIndex,
                    _In_ const std::string& ifName,
                    _In_ const Buffer& buffer);

            virtual ~EventPayloadPacket() = default;

        public:

            otai_object_id_t getPort() const;

            int getIfIndex() const;

            const std::string& getIfName() const;

            const Buffer& getBuffer() const;

        private:

            otai_object_id_t m_port;

            int m_ifIndex;

            std::string m_ifName;

            Buffer m_buffer;
    };
}
