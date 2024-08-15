#pragma once

#include "otairedis.h"

#include "swss/sal.h"

#include <string>

namespace syncd
{
    class CommandLineOptions
    {
        public:

            CommandLineOptions();

            virtual ~CommandLineOptions() = default;

        public:

            virtual std::string getCommandLineString() const;

        public:
            bool m_enableSyncMode;

            bool m_enableOtaiBulkSupport;

            otai_redis_communication_mode_t m_redisCommunicationMode;

            std::string m_profileMapFile;

            uint32_t m_globalContext;
			uint32_t m_loglevel;

            std::string m_contextConfig;
    };
}
