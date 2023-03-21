#pragma once

#include "lairedis.h"

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

            bool m_enableDiagShell;
            bool m_enableUnittests;

            /**
             * When set to true will enable DB vs ASIC consistency check after
             * comparison logic.
             */
            bool m_enableConsistencyCheck;

            bool m_enableSyncMode;

            bool m_enableLaiBulkSupport;

            lai_redis_communication_mode_t m_redisCommunicationMode;

            std::string m_profileMapFile;

            uint32_t m_globalContext;
			uint32_t m_loglevel;

            std::string m_contextConfig;

#ifdef LAITHRIFT
            bool m_runRPCServer;
#endif // LAITHRIFT

    };
}
