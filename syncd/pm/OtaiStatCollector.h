#pragma once

#include <vector>
#include <string>
#include <set>

#include "Collector.h"

namespace syncd
{
    class OtaiStatCollector : public Collector
    {
    public:

        OtaiStatCollector(
            _In_ otai_object_type_t objectType,
            _In_ otai_object_id_t vid,
            _In_ otai_object_id_t rid,
            std::shared_ptr<otairedis::OtaiInterface> vendorOtai,
            _In_ const std::set<std::string> &strStatIds);

        ~OtaiStatCollector();

        void collect();

    private:

        struct AccumulativeValue
        {
            bool m_init;

            uint64_t m_interval; /* seconds */

            uint64_t m_starttime;

            uint32_t m_expiretime;

            otai_stat_value_t m_stataccvalue;

            otai_stat_value_t m_statvaluedb;

            validity_type m_validityType;

            uint32_t m_failurecount;

            AccumulativeValue()
            {
                memset(&m_stataccvalue, 0, sizeof(otai_stat_value_t));
                m_validityType = VALIDITY_TYPE_INCOMPLETE;

                m_init = true;
                m_failurecount = 0;
            }
        };

        struct entry
        {
            const otai_stat_metadata_t *m_meta;

            otai_stat_id_t m_statid;

            otai_stat_value_t m_statvalue; /* from OTAI */

            AccumulativeValue m_accvalue;

            AccumulativeValue m_accvalue15min;

            AccumulativeValue m_accvalue24hour;

            entry(const otai_stat_metadata_t *meta)
                : m_meta(meta)
            {
                m_statid = meta->statid;
            }
        };

        std::vector<entry> m_entries;
           
        std::string m_keyCur;

        std::string m_key15min;

        std::string m_key24hour;

        std::string m_historyKey15min;

        std::string m_historyKey24hour;

        void updateCurrentValue(entry &e);

        void updatePeriodicValue(entry &e, StatisticalCycle cycle);

    };
}

