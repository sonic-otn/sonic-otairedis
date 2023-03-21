#include "ResourceLimiter.h"

#include "swss/logger.h"
#include "meta/lai_serialize.h"

using namespace laivs;

ResourceLimiter::ResourceLimiter(
        _In_ uint32_t linecardIndex):
    m_linecardIndex(linecardIndex)
{
    SWSS_LOG_ENTER();

    // empty
}

size_t ResourceLimiter::getObjectTypeLimit(
        _In_ lai_object_type_t objectType) const
{
    SWSS_LOG_ENTER();

    auto it = m_objectTypeLimits.find(objectType);

    if (it != m_objectTypeLimits.end())
    {
        return it->second;
    }

    // default limit is maximum

    return SIZE_MAX;
}

void ResourceLimiter::setObjectTypeLimit(
        _In_ lai_object_type_t objectType,
        _In_ size_t limit)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("setting %s limit to %zu",
            lai_serialize_object_type(objectType).c_str(),
            limit);

    m_objectTypeLimits[objectType] = limit;
}

void ResourceLimiter::removeObjectTypeLimit(
        _In_ lai_object_type_t objectType)
{
    SWSS_LOG_ENTER();

    m_objectTypeLimits[objectType] = SIZE_MAX;
}

void ResourceLimiter::clearLimits()
{
    SWSS_LOG_ENTER();

    m_objectTypeLimits.clear();
}
