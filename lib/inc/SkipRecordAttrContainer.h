#pragma once

extern "C" {
#include "lai.h"
}

#include <map>
#include <set>

namespace lairedis
{
    /**
     * @brief Skip record attributes container.
     *
     * This container will hold attributes that can be skipped when recording
     * GET api, for example statistics attributes and *_AVAILABLE_*.
     *
     * Since orch agent can query those frequently it can produce large
     * recording files without meaningful information.
     *
     * Any attribute can be added to this container, but only non oid attributes
     * will be accepted.
     */
    class SkipRecordAttrContainer
    {
        public:

            SkipRecordAttrContainer();

            virtual ~SkipRecordAttrContainer() = default;

        public:

            bool add(
                    _In_ lai_object_type_t objectType,
                    _In_ lai_attr_id_t attrId);

            bool remove(
                    _In_ lai_object_type_t objectType,
                    _In_ lai_attr_id_t attrId);

            void clear();

            bool canSkipRecording(
                    _In_ lai_object_type_t objectType,
                    _In_ uint32_t count,
                    _In_ lai_attribute_t* attrList) const;

        private:

            std::map<lai_object_type_t, std::set<lai_attr_id_t>> m_map;
    };
}
