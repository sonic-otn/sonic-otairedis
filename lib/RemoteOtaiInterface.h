#pragma once

#include "otairedis.h"

#include "meta/OtaiInterface.h"


namespace otairedis
{
    class RemoteOtaiInterface:
        public OtaiInterface
    {
        public:

            RemoteOtaiInterface() = default;

            virtual ~RemoteOtaiInterface() = default;
    };
}
