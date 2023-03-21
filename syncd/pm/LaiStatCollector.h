#pragma once

#include <vector>
#include <string>
#include <set>

#include "Collector.h"

namespace syncd
{
    class LaiStatCollector : public Collector
    {
    public:

        LaiStatCollector(
            _In_ lai_object_type_t objectType,
            _In_ lai_object_id_t vid,
            _In_ lai_object_id_t rid,
            std::shared_ptr<lairedis::LaiInterface> vendorLai,
            _In_ const std::set<std::string> &strStatIds);

        ~LaiStatCollector();

        void collect();

    private:

        struct AccumulativeValue
        {
            bool m_init;

            uint64_t m_interval; /* seconds */

            uint64_t m_starttime;

            uint32_t m_expiretime;

            lai_stat_value_t m_stataccvalue;

            lai_stat_value_t m_statvaluedb;

            validity_type m_validityType;

            uint32_t m_failurecount;

            AccumulativeValue()
            {
                memset(&m_stataccvalue, 0, sizeof(lai_stat_value_t));
                m_validityType = VALIDITY_TYPE_INCOMPLETE;

                m_init = true;
                m_failurecount = 0;
            }
        };

        struct entry
        {
            const lai_stat_metadata_t *m_meta;

            lai_stat_id_t m_statid;

            lai_stat_value_t m_statvalue; /* from LAI */

            AccumulativeValue m_accvalue;

            AccumulativeValue m_accvalue15min;

            AccumulativeValue m_accvalue24hour;

            entry(const lai_stat_metadata_t *meta)
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

