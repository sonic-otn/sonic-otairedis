#include "LaneMapContainer.h"

#include "swss/logger.h"

using namespace otaivs;

void LaneMapContainer::insert(
        _In_ std::shared_ptr<LaneMap> laneMap)
{
    SWSS_LOG_ENTER();

    m_map[laneMap->getLinecardIndex()] = laneMap;
}

void LaneMapContainer::remove(
        _In_ uint32_t linecardIndex)
{
    SWSS_LOG_ENTER();

    auto it = m_map.find(linecardIndex);

    if (it != m_map.end())
    {
        m_map.erase(it);
    }
}

std::shared_ptr<LaneMap> LaneMapContainer::getLaneMap(
        _In_ uint32_t linecardIndex) const
{
    SWSS_LOG_ENTER();

    auto it = m_map.find(linecardIndex);

    if (it == m_map.end())
    {
        return nullptr;
    }

    return it->second;
}

void LaneMapContainer::clear()
{
    SWSS_LOG_ENTER();

    m_map.clear();
}

bool LaneMapContainer::hasLaneMap(
        _In_ uint32_t linecardIndex) const
{
    SWSS_LOG_ENTER();

    return m_map.find(linecardIndex) != m_map.end();
}

size_t LaneMapContainer::size() const
{
    SWSS_LOG_ENTER();

    return m_map.size();
}

void LaneMapContainer::removeEmptyLaneMaps()
{
    SWSS_LOG_ENTER();

    for (auto it = m_map.begin(); it != m_map.end();)
    {
        if (it->second->isEmpty())
        {
            it = m_map.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
