#pragma once

#include "swss/sal.h"

#include <string>
#include <memory>

namespace otairedis
{
    class LinecardConfig
    {
        public:

            LinecardConfig();

            LinecardConfig(
                    _In_ uint32_t linecardIndex,
                    _In_ const std::string& hwinfo);

            virtual ~LinecardConfig() = default;

        public:

            uint32_t m_linecardIndex;

            std::string m_hardwareInfo;
    };
}
