#include "OtaiLinecard.h"
#include "VendorOtai.h"
#include "VidManager.h"
#include "RedisClient.h"

#include "meta/otai_serialize.h"
#include "swss/logger.h"

using namespace syncd;

/*
 * NOTE: If real ID will change during hard restarts, then we need to remap all
 * VID/RID, but we can only do that if we will save entire tree with all
 * dependencies.
 */

OtaiLinecard::OtaiLinecard(
        _In_ otai_object_id_t linecard_vid,
        _In_ otai_object_id_t linecard_rid,
        _In_ std::shared_ptr<RedisClient> client,
        _In_ std::shared_ptr<VirtualOidTranslator> translator,
        _In_ std::shared_ptr<otairedis::OtaiInterface> vendorOtai):
    m_linecard_vid(linecard_vid),
    m_linecard_rid(linecard_rid),
    m_client(client),
    m_translator(translator),
    m_vendorOtai(vendorOtai)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_TIMER("constructor");
}

std::unordered_map<otai_object_id_t, otai_object_id_t> OtaiLinecard::getVidToRidMap() const
{
    SWSS_LOG_ENTER();

    return m_client->getVidToRidMap(m_linecard_vid);
}

std::unordered_map<otai_object_id_t, otai_object_id_t> OtaiLinecard::getRidToVidMap() const
{
    SWSS_LOG_ENTER();

    return m_client->getRidToVidMap(m_linecard_vid);
}

otai_object_id_t OtaiLinecard::getRid() const
{
    SWSS_LOG_ENTER();

    return m_linecard_rid;
}

otai_object_id_t OtaiLinecard::getVid() const
{
    SWSS_LOG_ENTER();

    return m_linecard_vid;
}


