#pragma once

#include "CorePortIndexMap.h"

#include <memory>
#include <map>

namespace otaivs
{
    class CorePortIndexMapContainer
    {
        public:

            CorePortIndexMapContainer() = default;

            virtual ~CorePortIndexMapContainer() = default;

        public:

            void insert(
                    _In_ std::shared_ptr<CorePortIndexMap> corePortIndexMap);

            void remove(
                    _In_ uint32_t linecardIndex);

            std::shared_ptr<CorePortIndexMap> getCorePortIndexMap(
                    _In_ uint32_t linecardIndex) const;

            void clear();

            bool hasCorePortIndexMap(
                    _In_ uint32_t linecardIndex) const;

            size_t size() const;

            void removeEmptyCorePortIndexMaps();

        private:

            std::map<uint32_t, std::shared_ptr<CorePortIndexMap>> m_map;
    };
}

