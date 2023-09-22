#pragma once

#include "FlexCounter.h"

namespace syncd
{
    class FlexCounterManager
    {
    public:

        FlexCounterManager(
            _In_ std::shared_ptr<otairedis::OtaiInterface> vendorOtai,
            _In_ const std::string& dbCounters);

        virtual ~FlexCounterManager() = default;

    public:

        std::shared_ptr<FlexCounter> getInstance(
            _In_ const std::string& instanceId);

        void removeInstance(
            _In_ const std::string& instanceId);

        void removeAllCounters();

        void removeCounterPlugins(
            _In_ const std::string& instanceId);

        void addCounterPlugin(
            _In_ const std::string& instanceId,
            _In_ const std::vector<swss::FieldValueTuple>& values);

        void addCounter(
            _In_ otai_object_id_t vid,
            _In_ otai_object_id_t rid,
            _In_ const std::string& instanceId,
            _In_ const std::vector<swss::FieldValueTuple>& values);

        void removeCounter(
            _In_ otai_object_id_t vid,
            _In_ const std::string& instanceId);

    private:

        std::map<std::string, std::shared_ptr<FlexCounter>> m_flexCounters;

        std::mutex m_mutex;

        std::shared_ptr<otairedis::OtaiInterface> m_vendorOtai;

        std::string m_dbCounters;
        std::string m_dbState;
        std::string m_dbGBCounters;
    };
}

