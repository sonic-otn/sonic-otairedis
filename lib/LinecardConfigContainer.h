#pragma once

#include "LinecardConfig.h"

#include "swss/sal.h"

#include <map>
#include <memory>

namespace otairedis
{
    class LinecardConfigContainer
    {
        public:

            LinecardConfigContainer() = default;

            virtual ~LinecardConfigContainer() = default;

        public:

            void insert(
                    _In_ std::shared_ptr<LinecardConfig> config);

            std::shared_ptr<LinecardConfig> getConfig(
                    _In_ uint32_t linecardIndex) const;

            std::shared_ptr<LinecardConfig> getConfig(
                    _In_ const std::string& hardwareInfo) const;

        private:

            std::map<uint32_t, std::shared_ptr<LinecardConfig>> m_indexToConfig;

            std::map<std::string, std::shared_ptr<LinecardConfig>> m_hwinfoToConfig;
    };
}
