#pragma once

#include <memory>
#include <string>

#include "meta/otai_serialize.h"
#include "swss/dbconnector.h"
#include "swss/table.h"
#include "swss/logger.h"
#include "OtaiInterface.h"

namespace syncd
{

#define PM_CYCLE_1_SEC           (1000000000ull)
#define PM_CYCLE_15_MINS         (15 * 60 * PM_CYCLE_1_SEC)
#define PM_CYCLE_24_HOURS        (24 * 60 * 60 * PM_CYCLE_1_SEC)

#define EXPIRE_TIME_2_DAYS  (2 * 24 * 60 * 60)
#define EXPIRE_TIME_7_DAYS  (7 * 24 * 60 * 60)

    enum StatisticalCycle
    {
        STAT_CYCLE_15_MINS,
        STAT_CYCLE_24_HOURS,
    }; 

    class Collector
    {
    public:

        Collector(
            _In_ otai_object_type_t objectType,
            _In_ otai_object_id_t vid,
            _In_ otai_object_id_t rid,
            std::shared_ptr<otairedis::OtaiInterface> vendorOtai);

        virtual ~Collector();

        virtual void collect() = 0;

    protected:

        otai_object_type_t m_objectType;

        otai_object_id_t m_vid;

        otai_object_id_t m_rid;

        std::shared_ptr<otairedis::OtaiInterface> m_vendorOtai;         

        std::shared_ptr<swss::DBConnector> m_stateDb;

        std::shared_ptr<swss::DBConnector> m_countersDb;

        std::shared_ptr<swss::DBConnector> m_historyDb;

        std::unique_ptr<swss::Table> m_stateTable;

        std::string m_stateTableKeyName;

        std::unique_ptr<swss::Table> m_countersTable;

        std::string m_countersTableKeyName;

        std::string m_countersTableName;

        std::unique_ptr<swss::Table> m_historyTable;

        std::string m_historyTableKeyName;

    protected:

        uint64_t m_collectTime;

        long m_counter15min;

        long m_counter24hour;

        bool m_timeout15min;

        bool m_timeout24hour;

        void updateTimeFlags();

        double convertMilliWatt2dBm(double p);

        double convertdBm2MilliWatt(double x);

        enum validity_type
        {
            VALIDITY_TYPE_COMPLETE,
            VALIDITY_TYPE_INCOMPLETE,
            VALIDITY_TYPE_INVALID,
        };

        std::string validityToString(validity_type type)
        {
             switch (type)
             {
             case VALIDITY_TYPE_COMPLETE:
                 return "complete";
             case VALIDITY_TYPE_INCOMPLETE:
                 return "incomplete";
             case VALIDITY_TYPE_INVALID:
                 return "invalid";
             default:
                 break;
             }

             return "null";
        }

    };
}

