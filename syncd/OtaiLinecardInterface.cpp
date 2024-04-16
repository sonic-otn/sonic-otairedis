#include "OtaiLinecardInterface.h"

#include "swss/logger.h"

using namespace syncd;

OtaiLinecardInterface::OtaiLinecardInterface(
        _In_ otai_object_id_t linecardVid,
        _In_ otai_object_id_t linecardRid):
    m_linecard_vid(linecardVid),
    m_linecard_rid(linecardRid)
{
    SWSS_LOG_ENTER();

    // empty
}

otai_object_id_t OtaiLinecardInterface::getVid() const
{
    SWSS_LOG_ENTER();

    return m_linecard_vid;
}

otai_object_id_t OtaiLinecardInterface::getRid() const
{
    SWSS_LOG_ENTER();

    return m_linecard_rid;
}

otai_object_id_t OtaiLinecardInterface::getLinecardDefaultAttrOid(
        _In_ otai_attr_id_t attr_id) const
{
    SWSS_LOG_ENTER();

    auto it = m_default_rid_map.find(attr_id);

    if (it == m_default_rid_map.end())
    {
        auto meta = otai_metadata_get_attr_metadata(OTAI_OBJECT_TYPE_LINECARD, attr_id);

        const char* name = (meta) ? meta->attridname : "UNKNOWN";

        SWSS_LOG_THROW("attribute %s (%d) not found in default RID map", name, attr_id);
    }

    return it->second;
}

std::set<otai_object_id_t> OtaiLinecardInterface::getWarmBootNewDiscoveredVids()
{
    SWSS_LOG_ENTER();

    return m_warmBootNewDiscoveredVids;
}

