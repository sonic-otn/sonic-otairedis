#pragma once

#include <vector>
#include <string>
#include <set>

#include "Collector.h"

namespace syncd
{
    class LaiGaugeCollector : public Collector
    {
    public:

        LaiGaugeCollector(
            _In_ lai_object_type_t objectType,
            _In_ lai_object_id_t vid,
            _In_ lai_object_id_t rid,
            std::shared_ptr<lairedis::LaiInterface> vendorLai,
            _In_ const std::set<std::string> &strStatIds);

        ~LaiGaugeCollector();

        void collect();

    private:

        struct AvgMinMaxValue
        {
            bool m_init;

            lai_stat_value_t m_maxvalue;
            uint64_t m_maxtime;

            lai_stat_value_t m_minvalue;
            uint64_t m_mintime;

            lai_stat_value_t m_instantvalue;

            uint64_t m_starttime;

            uint64_t m_interval; /* seconds */

            uint32_t m_expiretime;

            // average value related elements
            lai_stat_value_t m_avgvalue;
            lai_stat_value_t m_accvalue;
            uint64_t m_accnum;

            validity_type m_validityType;

            validity_type m_currentValidityType;

            uint64_t m_failurecount;

            AvgMinMaxValue()
            {
                m_accnum = 0;
                m_init = true;
                m_failurecount = 0;
            }
        };

        struct entry
        {
            const lai_stat_metadata_t *m_meta;

            lai_stat_id_t m_statid;

            lai_stat_value_t m_statvalue;

            AvgMinMaxValue m_statvalue15min;

            AvgMinMaxValue m_statvalue24hour;

            std::string m_key15min;

            std::string m_key24hour;

            std::string m_historyKey15min;

            std::string m_historyKey24hour;

            entry(const lai_stat_metadata_t *meta, std::string &tableKeyName)
                : m_meta(meta)
            {
                m_statid = meta->statid;

                std::string keyHead = tableKeyName + "_" + 
                                      lai_serialize_stat_id_camel_case(*meta);

                m_key15min = keyHead + ":15_pm_current";
                m_key24hour = keyHead + ":24_pm_current";
                m_historyKey15min = keyHead + ":15_pm_history_";
                m_historyKey24hour = keyHead + ":24_pm_history_";

            }
        };

        std::vector<entry> m_entries;
           
        void updatePeriodicValue(entry &e, StatisticalCycle cycle); 

    };
}

