#pragma once

#include "swss/sal.h"

#include <inttypes.h>

#include <map>
#include <vector>
#include <string>
#include <memory>

// TODO port remove / create will need update this lane map

namespace laivs
{
    class LaneMap
    {
        public:

            constexpr static uint32_t DEFAULT_LINECARD_INDEX = 0;

        public:

            LaneMap(
                    _In_ uint32_t linecardIndex);

            virtual ~LaneMap() = default;

        public:

            bool add(
                    _In_ const std::string& ifname,
                    _In_ const std::vector<uint32_t>& lanes);

            bool remove(
                    _In_ const std::string& ifname);

            uint32_t getLinecardIndex() const;

            bool isEmpty() const;

            bool hasInterface(
                    _In_ const std::string& ifname) const;

            const std::vector<std::vector<uint32_t>> getLaneVector() const;

            /**
             * @brief Get interface from lane number.
             *
             * @return Interface name or empty string if lane number not found.
             */
            std::string getInterfaceFromLaneNumber(
                    _In_ uint32_t laneNumber) const;

        public:

            static std::shared_ptr<LaneMap> getDefaultLaneMap(
                    _In_ uint32_t linecardIndex = DEFAULT_LINECARD_INDEX);

        private:

            uint32_t m_linecardIndex;

            std::map<uint32_t, std::string> m_lane_to_ifname;

            std::map<std::string, std::vector<uint32_t>> m_ifname_to_lanes;

            std::vector<std::vector<uint32_t>> m_laneMap;
    };
}
