#include "LaiLinecardInterface.h"

#include "swss/logger.h"

using namespace syncd;

LaiLinecardInterface::LaiLinecardInterface(
        _In_ lai_object_id_t linecardVid,
        _In_ lai_object_id_t linecardRid):
    m_linecard_vid(linecardVid),
    m_linecard_rid(linecardRid)
{
    SWSS_LOG_ENTER();

    // empty
}

lai_object_id_t LaiLinecardInterface::getVid() const
{
    SWSS_LOG_ENTER();

    return m_linecard_vid;
}

lai_object_id_t LaiLinecardInterface::getRid() const
{
    SWSS_LOG_ENTER();

    return m_linecard_rid;
}

lai_object_id_t LaiLinecardInterface::getLinecardDefaultAttrOid(
        _In_ lai_attr_id_t attr_id) const
{
    SWSS_LOG_ENTER();

    auto it = m_default_rid_map.find(attr_id);

    if (it == m_default_rid_map.end())
    {
        auto meta = lai_metadata_get_attr_metadata(LAI_OBJECT_TYPE_LINECARD, attr_id);

        const char* name = (meta) ? meta->attridname : "UNKNOWN";

        SWSS_LOG_THROW("attribute %s (%d) not found in default RID map", name, attr_id);
    }

    return it->second;
}

std::set<lai_object_id_t> LaiLinecardInterface::getWarmBootNewDiscoveredVids()
{
    SWSS_LOG_ENTER();

    return m_warmBootNewDiscoveredVids;
}

