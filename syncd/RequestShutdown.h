#pragma once

#include "swss/sal.h"

#include <memory>

#define SYNCD_NOTIFICATION_CHANNEL_RESTARTQUERY     "RESTARTQUERY"

namespace syncd
{
    class RequestShutdown
    {
        public:

            RequestShutdown();

            virtual ~RequestShutdown();

        public:

            void send();

    };
}
