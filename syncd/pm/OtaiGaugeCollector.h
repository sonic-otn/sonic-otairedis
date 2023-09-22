#pragma once

#include <vector>
#include <string>
#include <set>

#include "Collector.h"

namespace syncd
{
    class OtaiGaugeCollector : public Collector
    {
    public:

        OtaiGaugeCollector(
            _In_ otai_object_type_t objectType,
            _In_ otai_object_id_t vid,
            _In_ otai_object_id_t rid,
            std::shared_ptr<otairedis::OtaiInterface> vendorOtai,
            _In_ const std::set<std::string> &strStatIds);

        ~OtaiGaugeCollector();

        void collect();

    private:

        struct AvgMinMaxValue
        {
            bool m_init;

            otai_stat_value_t m_maxvalue;
            uint64_t m_maxtime;

            otai_stat_value_t m_minvalue;
            uint64_t m_mintime;

            otai_stat_value_t m_instantvalue;

            uint64_t m_starttime;

            uint64_t m_interval; /* seconds */

            uint32_t m_expiretime;

            // average value related elements
            otai_stat_value_t m_avgvalue;
            otai_stat_value_t m_accvalue;
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
            const otai_stat_metadata_t *m_meta;

            otai_stat_id_t m_statid;

            otai_stat_value_t m_statvalue;

            AvgMinMaxValue m_statvalue15min;

            AvgMinMaxValue m_statvalue24hour;

            std::string m_key15min;

            std::string m_key24hour;

            std::string m_historyKey15min;

            std::string m_historyKey24hour;

            entry(const otai_stat_metadata_t *meta, std::string &tableKeyName)
                : m_meta(meta)
            {
                m_statid = meta->statid;

                std::string keyHead = tableKeyName + "_" + 
                                      otai_serialize_stat_id_camel_case(*meta);

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

