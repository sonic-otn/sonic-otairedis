#include "LinecardContainer.h"

#include "swss/logger.h"
#include "meta/otai_serialize.h"

using namespace otaivs;

void LinecardContainer::insert(
        _In_ std::shared_ptr<Linecard> sw)
{
    SWSS_LOG_ENTER();

    auto linecardId = sw->getLinecardId();

    if (m_linecardMap.find(linecardId) != m_linecardMap.end())
    {
        SWSS_LOG_THROW("linecard %s already exist in container",
                otai_serialize_object_id(linecardId).c_str());
    }

    m_linecardMap[linecardId] = sw;
}

void LinecardContainer::removeLinecard(
        _In_ otai_object_id_t linecardId)
{
    SWSS_LOG_ENTER();

    auto it = m_linecardMap.find(linecardId);

    if (it == m_linecardMap.end())
    {
        SWSS_LOG_THROW("linecard %s not present in container",
                otai_serialize_object_id(linecardId).c_str());
    }

    m_linecardMap.erase(it);
}

void LinecardContainer::removeLinecard(
        _In_ std::shared_ptr<Linecard> sw)
{
    SWSS_LOG_ENTER();

    removeLinecard(sw->getLinecardId());
}

std::shared_ptr<Linecard> LinecardContainer::getLinecard(
        _In_ otai_object_id_t linecardId) const
{
    SWSS_LOG_ENTER();

    auto it = m_linecardMap.find(linecardId);

    if (it == m_linecardMap.end())
        return nullptr;

    return it->second;
}

void LinecardContainer::clear()
{
    SWSS_LOG_ENTER();

    m_linecardMap.clear();
}

bool LinecardContainer::contains(
        _In_ otai_object_id_t linecardId) const
{
    SWSS_LOG_ENTER();

    return m_linecardMap.find(linecardId) != m_linecardMap.end();
}
