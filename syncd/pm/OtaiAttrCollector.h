#pragma once

#include <vector>
#include <set>
#include <string>

#include "Collector.h"

namespace syncd
{
    
    class OtaiAttrCollector : public Collector
    {
    public:

        OtaiAttrCollector(
            _In_ otai_object_type_t objectType,
            _In_ otai_object_id_t vid,
            _In_ otai_object_id_t rid,
            std::shared_ptr<otairedis::OtaiInterface> vendorOtai,
            _In_ const std::set<std::string> &strAttrIds);

        ~OtaiAttrCollector();

        void collect();

    private:

        struct entry
        {
            bool m_init;

            const otai_attr_metadata_t *m_meta;

            otai_attribute_t m_attr;

            otai_attribute_t m_attrdb;

            entry(const otai_attr_metadata_t *meta)
                : m_meta(meta)
            {
                m_attr.id = meta->attrid;
                m_attrdb.id = meta->attrid;

                m_init = true;
            }
        };

        std::vector<entry> m_entries;

        void newOtaiAttr(
            _Inout_ otai_attribute_t &attr,
            _In_ const otai_attr_metadata_t *meta);

        void freeOtaiAttr(
            _Inout_ otai_attribute_t &attr,
            _In_ const otai_attr_metadata_t *meta);

        void updateCurrentValue(entry &e);
    };
}

