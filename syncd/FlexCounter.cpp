#include <cinttypes>
#include <chrono>
#include <cstdio>
#include <cstdlib>

#include "FlexCounter.h"
#include "VidManager.h"

#include "meta/otai_serialize.h"
#include "meta/OtaiInterface.h"

#include "swss/redisapi.h"
#include "swss/tokenize.h"

using namespace syncd;

#define MUTEX std::unique_lock<std::mutex> _lock(m_mtx);
#define MUTEX_UNLOCK _lock.unlock();

FlexCounter::FlexCounter(
    _In_ const std::string& instanceId,
    _In_ std::shared_ptr<otairedis::OtaiInterface> vendorOtai,
    _In_ const std::string& dbCounters):
    m_pollInterval(0),
    m_instanceId(instanceId),
    m_vendorOtai(vendorOtai)
{
    SWSS_LOG_ENTER();

    m_propGroup = OTAI_PROPERTY_GROUP_NULL;

    if (string::npos != m_instanceId.find("1S_STAT_STATUS"))
    {
        m_propGroup = OTAI_PROPERTY_GROUP_ATTR;
    }
    else if (string::npos != m_instanceId.find("1S_STAT_GAUGE"))
    {
        m_propGroup = OTAI_PROPERTY_GROUP_GAUGE;
    }
    else if (string::npos != m_instanceId.find("1S_STAT_COUNTER"))
    {
        m_propGroup = OTAI_PROPERTY_GROUP_STAT;
    }

    m_enable = false;
    m_isDiscarded = false;

    startFlexCounterThread();
}

FlexCounter::~FlexCounter(void)
{
    SWSS_LOG_ENTER();

    endFlexCounterThread();

    MUTEX;

    for (auto c = m_collectors.begin(); c != m_collectors.end(); c++)
    {
        delete c->second;
    }

}

void FlexCounter::setPollInterval(
    _In_ uint32_t pollInterval)
{
    SWSS_LOG_ENTER();

    m_pollInterval = pollInterval;
}

void FlexCounter::setStatus(
    _In_ const std::string& status)
{
    SWSS_LOG_ENTER();

    if (status == "enable")
    {
        m_enable = true;
    }
    else if (status == "disable")
    {
        m_enable = false;
    }
    else
    {
        SWSS_LOG_WARN("Input value %s is not supported for Flex counter status, enter enable or disable", status.c_str());
    }
}

void FlexCounter::setStatsMode(
    _In_ const std::string& mode)
{
    SWSS_LOG_ENTER();

    if (mode == STATS_MODE_READ)
    {
        m_statsMode = OTAI_STATS_MODE_READ;

        SWSS_LOG_DEBUG("Set STATS MODE %s for instance %s", mode.c_str(), m_instanceId.c_str());
    }
    else if (mode == STATS_MODE_READ_AND_CLEAR)
    {
        m_statsMode = OTAI_STATS_MODE_READ_AND_CLEAR;

        SWSS_LOG_DEBUG("Set STATS MODE %s for instance %s", mode.c_str(), m_instanceId.c_str());
    }
    else
    {
        SWSS_LOG_WARN("Input value %s is not supported for Flex counter stats mode, enter STATS_MODE_READ or STATS_MODE_READ_AND_CLEAR", mode.c_str());
    }
}

void FlexCounter::addCollectCountersHandler(const std::string& key, const collect_counters_handler_t& handler)
{
    SWSS_LOG_ENTER();

    m_collectCountersHandlers.emplace(key, handler);
}

void FlexCounter::removeCollectCountersHandler(const std::string& key)
{
    SWSS_LOG_ENTER();

    m_collectCountersHandlers.erase(key);
}

void FlexCounter::checkPluginRegistered(
    _In_ const std::string& sha) const
{
    SWSS_LOG_ENTER();

}

void FlexCounter::removeCounterPlugins()
{
    MUTEX;

    SWSS_LOG_ENTER();

    m_isDiscarded = true;
}

void FlexCounter::addCounterPlugin(
    _In_ const std::vector<swss::FieldValueTuple>& values)
{
    MUTEX;

    SWSS_LOG_ENTER();

    m_isDiscarded = false;

    for (auto& fvt : values)
    {
        auto& field = fvField(fvt);
        auto& value = fvValue(fvt);

        auto shaStrings = swss::tokenize(value, ',');

        if (field == POLL_INTERVAL_FIELD)
        {
            setPollInterval(stoi(value));
        }
        else if (field == FLEX_COUNTER_STATUS_FIELD)
        {
            setStatus(value);
        }
        else if (field == STATS_MODE_FIELD)
        {
            setStatsMode(value);
        }
        else
        {
            SWSS_LOG_ERROR("Field is not supported %s", field.c_str());
        }
    }

    // notify thread to start polling
    m_pollCond.notify_all();
}

bool FlexCounter::isEmpty()
{
    MUTEX;

    SWSS_LOG_ENTER();

    return allIdsEmpty() && allPluginsEmpty();
}

bool FlexCounter::isDiscarded()
{
    SWSS_LOG_ENTER();

    return isEmpty() && m_isDiscarded;
}

bool FlexCounter::allIdsEmpty()
{
    SWSS_LOG_ENTER();
    return m_collectors.empty();
}

bool FlexCounter::allPluginsEmpty() const
{
    SWSS_LOG_ENTER();

    return true;
}

void FlexCounter::collectCounters()
{
    SWSS_LOG_ENTER();

    for (auto &c : m_collectors)
    {
        c.second->collect();
    }
}

void FlexCounter::runPlugins(
    _In_ swss::DBConnector& counters_db)
{
    SWSS_LOG_ENTER();
}

void FlexCounter::flexCounterThreadRunFunction()
{
    SWSS_LOG_ENTER();

    while (m_runFlexCounterThread)
    {
        MUTEX;

        if (m_enable && !allIdsEmpty() && (m_pollInterval > 0))
        {
            auto start = std::chrono::steady_clock::now();

            collectCounters();

            auto finish = std::chrono::steady_clock::now();

            uint32_t delay = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count());

            uint32_t correction = delay % m_pollInterval;
            correction = m_pollInterval - correction;
            MUTEX_UNLOCK; // explicit unlock

            SWSS_LOG_DEBUG("End of Flex_Counter cycle [%s], took %d ms / interval %d ms", m_instanceId.c_str(), delay, m_pollInterval);

            std::unique_lock<std::mutex> lk(m_mtxSleep);
            m_cvSleep.wait_for(lk, std::chrono::milliseconds(correction));
        }
        else
        {
            MUTEX_UNLOCK; // explicit unlock

            SWSS_LOG_DEBUG("End of Flex_Counter cycle [%s], nothing to collect, enable %d empty %d m_pollInterval %d", m_instanceId.c_str(), m_enable, allIdsEmpty(), m_pollInterval);
            // nothing to collect, wait until notified
            std::unique_lock<std::mutex> lk(m_mtxSleep);
            m_pollCond.wait(lk); // wait on mutex
        }
    }
}

void FlexCounter::startFlexCounterThread()
{
    SWSS_LOG_ENTER();

    m_runFlexCounterThread = true;

    m_flexCounterThread = std::make_shared<std::thread>(&FlexCounter::flexCounterThreadRunFunction, this);

    SWSS_LOG_NOTICE("Flex_Counter thread started.");
}

void FlexCounter::endFlexCounterThread(void)
{
    SWSS_LOG_ENTER();

    MUTEX;

    if (m_runFlexCounterThread)
    {
        m_runFlexCounterThread = false;

        m_pollCond.notify_all();

        m_cvSleep.notify_all();

        if (m_flexCounterThread != nullptr)
        {
            auto fcThread = std::move(m_flexCounterThread);

            MUTEX_UNLOCK; // NOTE: explicit unlock before join to not cause deadlock

            SWSS_LOG_NOTICE("Wait for Flex_Counter thread to end");

            fcThread->join();
        }

        SWSS_LOG_NOTICE("Flex_Counter thread ended");
    }
}

void FlexCounter::removeCounter(
    _In_ otai_object_id_t vid)
{
    MUTEX;

    SWSS_LOG_ENTER();

    auto it = m_collectors.find(vid);
    if (it != m_collectors.end())
    {
        delete it->second;
        m_collectors.erase(it);
    }
}

void FlexCounter::addCounter(
    _In_ otai_object_id_t vid,
    _In_ otai_object_id_t rid,
    _In_ const std::vector<swss::FieldValueTuple>& values)
{
    MUTEX;

    SWSS_LOG_ENTER();

    otai_object_type_t objectType = VidManager::objectTypeQuery(vid); // VID and RID will have the same object type

    for (const auto& valuePair : values)
    {
        const auto field = fvField(valuePair);

        const auto value = fvValue(valuePair);
        auto idStrings = swss::tokenize(value, ',');

        std::set<std::string> counterIds(idStrings.begin(), idStrings.end()); 

        SWSS_LOG_NOTICE("Object type %s rid 0x%" PRIx64 " m_propGroup %d",
                        otai_serialize_object_type(objectType).c_str(), rid, (int)m_propGroup);
        
        Collector *c = NULL;

        if (m_propGroup == OTAI_PROPERTY_GROUP_ATTR)
        {
            c = new OtaiAttrCollector(objectType, vid, rid, m_vendorOtai, counterIds);
        }
        else if (m_propGroup == OTAI_PROPERTY_GROUP_STAT)
        {
            c = new OtaiStatCollector(objectType, vid, rid, m_vendorOtai, counterIds);
        }
        else if (m_propGroup == OTAI_PROPERTY_GROUP_GAUGE)
        {
            c = new OtaiGaugeCollector(objectType, vid, rid, m_vendorOtai, counterIds);
        }

        if (c != NULL)
        {
            m_collectors[vid] = c;
        }
    }

    // notify thread to start polling
    m_pollCond.notify_all();
}

