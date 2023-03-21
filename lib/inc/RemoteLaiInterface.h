#pragma once

#include "LaiInterface.h"

namespace lairedis
{
    class RemoteLaiInterface:
        public LaiInterface
    {
        public:

            RemoteLaiInterface() = default;

            virtual ~RemoteLaiInterface() = default;

        public:

            /**
             * @brief Notify syncd API.
             */
            virtual lai_status_t notifySyncd(
                    _In_ lai_object_id_t switchId,
                    _In_ lai_redis_notify_syncd_t redisNotifySyncd) = 0;
    };
}
