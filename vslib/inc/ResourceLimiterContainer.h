#pragma once

#include "ResourceLimiter.h"

#include <memory>

namespace laivs
{
    class ResourceLimiterContainer
    {
        public:

            ResourceLimiterContainer() = default;

            virtual ~ResourceLimiterContainer() = default;

        public:

            void insert(
                    _In_ uint32_t linecardIndex,
                    _In_ std::shared_ptr<ResourceLimiter> rl);

            void remove(
                    _In_ uint32_t linecardIndex);

            std::shared_ptr<ResourceLimiter> getResourceLimiter(
                    _In_ uint32_t linecardIndex) const;

            void clear();

        private:

            std::map<uint32_t, std::shared_ptr<ResourceLimiter>> m_container;
    };
}
