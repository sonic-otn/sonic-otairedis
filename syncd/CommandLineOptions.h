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
            bool m_enableOtaiBulkSupport;

            std::string m_profileMapFile;

			uint32_t m_loglevel;
    };
}
