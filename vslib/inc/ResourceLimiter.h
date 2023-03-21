#pragma once

extern "C" {
#include "lai.h"
}

#include <map>

namespace laivs
{
    class ResourceLimiter
    {
        public:

             constexpr static uint32_t DEFAULT_LINECARD_INDEX = 0;

        public:

            ResourceLimiter(
                    _In_ uint32_t linecardIndex);

            virtual ~ResourceLimiter() = default;

        public:

            size_t getObjectTypeLimit(
                    _In_ lai_object_type_t objectType) const;

            void setObjectTypeLimit(
                    _In_ lai_object_type_t objectType,
                    _In_ size_t limit);

            void removeObjectTypeLimit(
                    _In_ lai_object_type_t objectType);

            void clearLimits();

        private:

            uint32_t m_linecardIndex;

            std::map<lai_object_type_t, size_t> m_objectTypeLimits;
    };
}
