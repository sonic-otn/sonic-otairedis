#pragma once

#include "RequestShutdownCommandLineOptions.h"
#include "ContextConfig.h"

#include "swss/sal.h"

#include <memory>

#define SYNCD_NOTIFICATION_CHANNEL_RESTARTQUERY     "RESTARTQUERY"

namespace syncd
{
    class RequestShutdown
    {
        public:

            RequestShutdown(
                    _In_ std::shared_ptr<RequestShutdownCommandLineOptions> options);

            virtual ~RequestShutdown();

        public:

            void send();

        private:

            std::shared_ptr<RequestShutdownCommandLineOptions> m_options;

            std::shared_ptr<lairedis::ContextConfig> m_contextConfig;
    };
}
