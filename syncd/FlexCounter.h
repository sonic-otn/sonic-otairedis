#pragma once

#include <vector>
#include <set>
#include <condition_variable>
#include <map>
#include <memory>
#include <string>
#include <mutex>

extern "C" {
#include "lai.h"
}

#include "LaiInterface.h"

#include "swss/table.h"

#include "pm/Collector.h"
#include "pm/LaiAttrCollector.h"
#include "pm/LaiStatCollector.h"
#include "pm/LaiGaugeCollector.h"

using namespace std;

namespace syncd
{
    enum lai_property_group_t
    {
        LAI_PROPERTY_GROUP_NULL,
        LAI_PROPERTY_GROUP_ATTR,
        LAI_PROPERTY_GROUP_GAUGE,
        LAI_PROPERTY_GROUP_STAT,
        LAI_PROPERTY_GROUP_MAX,
    };

    class FlexCounter
    {
    private:

        FlexCounter(const FlexCounter&) = delete;

    public:

        FlexCounter(
            _In_ const std::string& instanceId,
            _In_ std::shared_ptr<lairedis::LaiInterface> vendorLai,
            _In_ const std::string& dbCounters);

        virtual ~FlexCounter();

    public:

        void addCounterPlugin(
            _In_ const std::vector<swss::FieldValueTuple>& values);

        void removeCounterPlugins();

        void addCounter(
            _In_ lai_object_id_t vid,
            _In_ lai_object_id_t rid,
            _In_ const std::vector<swss::FieldValueTuple>& values);

        void removeCounter(
            _In_ lai_object_id_t vid);

        bool isEmpty();

        bool isDiscarded();

    private:

        void setPollInterval(
            _In_ uint32_t pollInterval);

        void setStatus(
            _In_ const std::string& status);

        void setStatsMode(
            _In_ const std::string& mode);

    private:

        void checkPluginRegistered(
            _In_ const std::string& sha) const;

        bool allIdsEmpty();

        bool allPluginsEmpty() const;

    private:

        void collectCounters();

        void runPlugins(_In_ swss::DBConnector& db);

        void startFlexCounterThread();

        void endFlexCounterThread();

        void flexCounterThreadRunFunction();

    private:

        typedef void (FlexCounter::* collect_counters_handler_t)(
            _In_ swss::Table& countersTable);

        typedef std::unordered_map<std::string, collect_counters_handler_t> collect_counters_handler_unordered_map_t;

    private: // collect counters:

        void addCollectCountersHandler(
            _In_ const std::string& key,
            _In_ const collect_counters_handler_t& handler);

        void removeCollectCountersHandler(
            _In_ const std::string& key);

    private:

        bool m_runFlexCounterThread;

        std::shared_ptr<std::thread> m_flexCounterThread;

        std::mutex m_mtxSleep;

        std::condition_variable m_cvSleep;

        std::mutex m_mtx;

        std::condition_variable m_pollCond;

        uint32_t m_pollInterval;

        std::string m_instanceId;

        lai_stats_mode_t m_statsMode;

        bool m_enable;

        collect_counters_handler_unordered_map_t m_collectCountersHandlers;

        std::shared_ptr<lairedis::LaiInterface> m_vendorLai;

        map<lai_object_id_t, Collector*> m_collectors;

        bool m_isDiscarded;

        lai_property_group_t m_propGroup;
    };
}

