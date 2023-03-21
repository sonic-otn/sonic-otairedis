#include "ResourceLimiterContainer.h"

#include "swss/logger.h"

using namespace laivs;

void ResourceLimiterContainer::insert(
        _In_ uint32_t linecardIndex,
        _In_ std::shared_ptr<ResourceLimiter> rl)
{
    SWSS_LOG_ENTER();

    m_container[linecardIndex] = rl;
}

void ResourceLimiterContainer::remove(
        _In_ uint32_t linecardIndex)
{
    SWSS_LOG_ENTER();

    auto it = m_container.find(linecardIndex);

    if (it != m_container.end())
    {
        m_container.erase(it);
    }
}

std::shared_ptr<ResourceLimiter> ResourceLimiterContainer::getResourceLimiter(
        _In_ uint32_t linecardIndex) const
{
    SWSS_LOG_ENTER();

    auto it = m_container.find(linecardIndex);

    if (it != m_container.end())
    {
        return it->second;
    }

    return nullptr;
}

void ResourceLimiterContainer::clear()
{
    SWSS_LOG_ENTER();

    m_container.clear();
}

