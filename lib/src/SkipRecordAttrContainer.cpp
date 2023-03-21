#include "SkipRecordAttrContainer.h"

extern "C" {
#include "laimetadata.h"
}

#include "swss/logger.h"

using namespace lairedis;

SkipRecordAttrContainer::SkipRecordAttrContainer()
{
    SWSS_LOG_ENTER();

    // default set of attributes to skip recording

}

bool SkipRecordAttrContainer::add(
        _In_ lai_object_type_t objectType,
        _In_ lai_attr_id_t attrId)
{
    SWSS_LOG_ENTER();

    auto md = lai_metadata_get_attr_metadata(objectType, attrId);

    if (md == NULL)
    {
        SWSS_LOG_WARN("failed to get metadata for %d:%d", objectType, attrId);

        return false;
    }

    if (md->isoidattribute)
    {
        SWSS_LOG_WARN("%s is OID attribute, will not add to container", md->attridname);

        return false;
    }

    m_map[objectType].insert(attrId);

    SWSS_LOG_DEBUG("added %s to container", md->attridname);

    return true;
}

bool SkipRecordAttrContainer::remove(
        _In_ lai_object_type_t objectType,
        _In_ lai_attr_id_t attrId)
{
    SWSS_LOG_ENTER();

    auto it = m_map.find(objectType);

    if (it == m_map.end())
    {
        return false;
    }

    auto its = it->second.find(attrId);

    if (its == it->second.end())
    {
        return false;
    }

    it->second.erase(its);

    return true;
}

void SkipRecordAttrContainer::clear()
{
    SWSS_LOG_ENTER();

    m_map.clear();
}

bool SkipRecordAttrContainer::canSkipRecording(
        _In_ lai_object_type_t objectType,
        _In_ uint32_t count,
        _In_ lai_attribute_t* attrList) const
{
    SWSS_LOG_ENTER();

    if (count == 0)
        return false;

    if (!attrList)
        return false;

    auto it = m_map.find(objectType);

    if (it == m_map.end())
        return false;

    // we can skip if all attributes on list are present in container

    auto& set = it->second;

    for (uint32_t idx = 0; idx < count; idx++)
    {
        if (set.find(attrList[idx].id) == set.end())
            return false;
    }

    return true;
}

