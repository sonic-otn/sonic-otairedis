#pragma once

#include "LaneMap.h"

#include <memory>
#include <map>

namespace otaivs
{
    class LaneMapContainer
    {
        public:

            LaneMapContainer() = default;

            virtual ~LaneMapContainer() = default;

        public:

            void insert(
                    _In_ std::shared_ptr<LaneMap> laneMap);

            void remove(
                    _In_ uint32_t linecardIndex);

            std::shared_ptr<LaneMap> getLaneMap(
                    _In_ uint32_t linecardIndex) const;

            void clear();

            bool hasLaneMap(
                    _In_ uint32_t linecardIndex) const;

            size_t size() const;

            void removeEmptyLaneMaps();

        private:

            std::map<uint32_t, std::shared_ptr<LaneMap>> m_map;
    };
}

