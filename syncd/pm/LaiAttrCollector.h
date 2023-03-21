#pragma once

#include <vector>
#include <set>
#include <string>

#include "Collector.h"

namespace syncd
{
    
    class LaiAttrCollector : public Collector
    {
    public:

        LaiAttrCollector(
            _In_ lai_object_type_t objectType,
            _In_ lai_object_id_t vid,
            _In_ lai_object_id_t rid,
            std::shared_ptr<lairedis::LaiInterface> vendorLai,
            _In_ const std::set<std::string> &strAttrIds);

        ~LaiAttrCollector();

        void collect();

    private:

        struct entry
        {
            bool m_init;

            const lai_attr_metadata_t *m_meta;

            lai_attribute_t m_attr;

            lai_attribute_t m_attrdb;

            entry(const lai_attr_metadata_t *meta)
                : m_meta(meta)
            {
                m_attr.id = meta->attrid;
                m_attrdb.id = meta->attrid;

                m_init = true;
            }
        };

        std::vector<entry> m_entries;

        void newLaiAttr(
            _Inout_ lai_attribute_t &attr,
            _In_ const lai_attr_metadata_t *meta);

        void freeLaiAttr(
            _Inout_ lai_attribute_t &attr,
            _In_ const lai_attr_metadata_t *meta);

        void updateCurrentValue(entry &e);
    };
}

