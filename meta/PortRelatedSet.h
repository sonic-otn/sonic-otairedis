#pragma once

extern "C" {
#include "lai.h"
}

#include <map>
#include <set>
#include <vector>

namespace laimeta
{
    class PortRelatedSet
    {
        public:

            PortRelatedSet() = default;

            virtual ~PortRelatedSet() = default;

        public:

            void insert(
                    _In_ lai_object_id_t portId,
                    _In_ lai_object_id_t relatedObjectId);

            const std::set<lai_object_id_t> getPortRelatedObjects(
                    _In_ lai_object_id_t portId) const;

            void clear();

            void removePort(
                    _In_ lai_object_id_t portId);

            std::vector<lai_object_id_t> getAllPorts() const;

        private:

            std::map<lai_object_id_t, std::set<lai_object_id_t>> m_mapset;
    };
}
