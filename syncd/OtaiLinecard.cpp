#include "OtaiLinecard.h"
#include "VendorOtai.h"
#include "OtaiDiscovery.h"
#include "VidManager.h"
#include "GlobalLinecardId.h"
#include "RedisClient.h"

#include "meta/otai_serialize.h"
#include "swss/logger.h"

using namespace syncd;

#define MAX_OBJLIST_LEN 128

#define MAX_LANES_PER_PORT 8

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
        _In_ std::shared_ptr<otairedis::OtaiInterface> vendorOtai,
        _In_ bool warmBoot):
    OtaiLinecardInterface(linecard_vid, linecard_rid),
    m_vendorOtai(vendorOtai),
    m_warmBoot(warmBoot),
    m_translator(translator),
    m_client(client)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_TIMER("constructor");

    GlobalLinecardId::setLinecardId(m_linecard_rid);

    /*
     * Discover put objects to redis needs to be called before checking lane
     * map and ports, since it will deduce whether put discovered objects to
     * redis to not interfere with possible user created objects previously.
     *
     * TODO: When user will use otairedis we need to send discovered view
     * with all objects dependencies to otairedis so metadata db could
     * be populated, and all references could be increased.
     */

    helperDiscover();

    helperSaveDiscoveredObjectsToRedis();

    helperInternalOids();

    helperLoadColdVids();

    helperPopulateWarmBootVids();

    if (warmBoot)
    {
        checkWarmBootDiscoveredRids();
    }
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

void OtaiLinecard::redisSetDummyAsicStateForRealObjectId(
        _In_ otai_object_id_t rid) const
{
    SWSS_LOG_ENTER();

    otai_object_id_t vid = m_translator->translateRidToVid(rid, m_linecard_vid);

    m_client->setDummyAsicStateObject(vid);
}

bool OtaiLinecard::isDiscoveredRid(
        _In_ otai_object_id_t rid) const
{
    SWSS_LOG_ENTER();

    return m_discovered_rids.find(rid) != m_discovered_rids.end();
}

std::set<otai_object_id_t> OtaiLinecard::getDiscoveredRids() const
{
    SWSS_LOG_ENTER();

    return m_discovered_rids;
}

void OtaiLinecard::removeExistingObjectReference(
        _In_ otai_object_id_t rid)
{
    SWSS_LOG_ENTER();

    auto it = m_discovered_rids.find(rid);

    if (it == m_discovered_rids.end())
    {
        SWSS_LOG_THROW("unable to find existing RID %s",
                otai_serialize_object_id(rid).c_str());
    }

    SWSS_LOG_INFO("removing ref RID %s",
            otai_serialize_object_id(rid).c_str());

    m_discovered_rids.erase(it);
}

void OtaiLinecard::removeExistingObject(
        _In_ otai_object_id_t rid)
{
    SWSS_LOG_ENTER();

    auto it = m_discovered_rids.find(rid);

    if (it == m_discovered_rids.end())
    {
        SWSS_LOG_THROW("unable to find existing RID %s",
                otai_serialize_object_id(rid).c_str());
    }

    otai_object_type_t ot = m_vendorOtai->objectTypeQuery(rid);

    if (ot == OTAI_OBJECT_TYPE_NULL)
    {
        SWSS_LOG_THROW("m_vendorOtai->objectTypeQuery returned NULL on RID %s",
                otai_serialize_object_id(rid).c_str());
    }

    auto info = otai_metadata_get_object_type_info(ot);

    otai_object_meta_key_t meta_key = { .objecttype = ot, .objectkey = {.key = { .object_id = rid } } };

    SWSS_LOG_INFO("removing %s", otai_serialize_object_meta_key(meta_key).c_str());

    otai_status_t status = m_vendorOtai->remove(meta_key.objecttype, meta_key.objectkey.key.object_id);

    if (status == OTAI_STATUS_SUCCESS)
    {
        m_discovered_rids.erase(it);
    }
    else
    {
        SWSS_LOG_ERROR("failed to remove %s RID %s: %s",
                info->objecttypename,
                otai_serialize_object_id(rid).c_str(),
                otai_serialize_status(status).c_str());
    }
}

/**
 * @brief Helper function to get attribute oid from linecard.
 *
 * Helper will try to obtain oid value for given attribute id.  On success, it
 * will try to obtain this value from redis database.  When value is not in
 * redis yet, it will store it, but when value was already there, it will
 * compare redis value to current oid and when they are different, it will
 * throw exception requesting for fix. When oid values are equal, function
 * returns current value.
 *
 * @param attr_id Attribute id to obtain oid from it.
 *
 * @return Valid object id (rid) if present, OTAI_NULL_OBJECT_ID on failure.
 */
otai_object_id_t OtaiLinecard::helperGetLinecardAttrOid(
        _In_ otai_attr_id_t attr_id)
{
    SWSS_LOG_ENTER();

    otai_attribute_t attr;
    memset(&attr, 0, sizeof(attr));
    /*
     * Get original value from the ASIC.
     */

    auto meta = otai_metadata_get_attr_metadata(OTAI_OBJECT_TYPE_LINECARD, attr_id);

    if (meta == NULL)
    {
        SWSS_LOG_THROW("can't get linecard attribute %d metadata", attr_id);
    }

    if (meta->attrvaluetype != OTAI_ATTR_VALUE_TYPE_OBJECT_ID)
    {
        SWSS_LOG_THROW("atribute %s is not OID attribute", meta->attridname);
    }

    attr.id = attr_id;

    otai_status_t status = m_vendorOtai->get(OTAI_OBJECT_TYPE_LINECARD, m_linecard_rid, 1, &attr);

    if (status != OTAI_STATUS_SUCCESS)
    {
        SWSS_LOG_WARN("failed to get %s: %s",
                meta->attridname,
                otai_serialize_status(status).c_str());

        return OTAI_NULL_OBJECT_ID;
    }

    SWSS_LOG_INFO("%s RID %s",
            meta->attridname,
            otai_serialize_object_id(attr.value.oid).c_str());

    otai_object_id_t rid = attr.value.oid;

    otai_object_id_t redis_rid = OTAI_NULL_OBJECT_ID;

    if (rid == OTAI_NULL_OBJECT_ID)
    {
        return rid;
    }

    /*
     * Get value value of the same attribute from redis.
     */

    auto ptr_redis_rid_str = m_client->getLinecardHiddenAttribute(m_linecard_vid, meta->attridname);

    if (ptr_redis_rid_str == NULL)
    {
        /*
         * Redis value of this attribute is not present yet, save it!
         */

        redisSaveInternalOids(rid);

        SWSS_LOG_INFO("redis %s id is not defined yet in redis", meta->attridname);

        m_client->saveLinecardHiddenAttribute(m_linecard_vid, meta->attridname, rid);

        m_default_rid_map[attr_id] = rid;

        return rid;
    }

    otai_deserialize_object_id(*ptr_redis_rid_str, redis_rid);

    if (rid != redis_rid)
    {
        /*
         * In that case we need to remap VIDTORID and RIDTOVID. This is
         * required since if previous value will be used in redis maps, and it
         * will be invalid when that value will be used to call OTAI api.
         */

        SWSS_LOG_THROW("FIXME: %s RID differs: %s (asic) vs %s (redis), ids must be remapped v2r/r2v",
                meta->attridname,
                otai_serialize_object_id(rid).c_str(),
                otai_serialize_object_id(redis_rid).c_str());
    }

    m_default_rid_map[attr_id] = rid;

    return rid;
}

bool OtaiLinecard::isColdBootDiscoveredRid(
        _In_ otai_object_id_t rid) const
{
    SWSS_LOG_ENTER();

    auto coldBootDiscoveredVids = getColdBootDiscoveredVids();

    /*
     * If object was discovered in cold boot, it must have valid RID assigned,
     * except objects that were removed like VLAN_MEMBER.
     */

    otai_object_id_t vid = m_translator->translateRidToVid(rid, m_linecard_vid);

    return coldBootDiscoveredVids.find(vid) != coldBootDiscoveredVids.end();
}

bool OtaiLinecard::isLinecardObjectDefaultRid(
        _In_ otai_object_id_t rid) const
{
    SWSS_LOG_ENTER();

    for (const auto &p: m_default_rid_map)
    {
        if (p.second == rid)
        {
            return true;
        }
    }

    return false;
}

bool OtaiLinecard::isNonRemovableRid(
        _In_ otai_object_id_t rid) const
{
    SWSS_LOG_ENTER();

    if (rid == OTAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("NULL rid passed");
    }

    /*
     * Check for OTAI_LINECARD_ATTR_DEFAULT_* oids like cpu, default virtual
     * router.  Those objects can't be removed if user ask for it.
     */

    /* Here we are checking for isLinecardObjectDefaultRid first then ColdBootDiscoveredRid
     * as it is possible we can discover linecard Internal OID as part of warm-boot also especially
     * when we are doing OTAI upgrade as part of warm-boot.*/

    if (isLinecardObjectDefaultRid(rid))
    {
        return true;
    }

    if (!isColdBootDiscoveredRid(rid))
    {
        /*
         * This object was not discovered on cold boot so it can be removed.
         */

        return false;
    }

    otai_object_type_t ot = m_vendorOtai->objectTypeQuery(rid);

    SWSS_LOG_WARN("can't determine wheter object %s RID %s can be removed, FIXME",
            otai_serialize_object_type(ot).c_str(),
            otai_serialize_object_id(rid).c_str());

    return true;
}

void OtaiLinecard::helperDiscover()
{
    SWSS_LOG_ENTER();

    OtaiDiscovery sd(m_vendorOtai);

    m_discovered_rids = sd.discover(m_linecard_rid);

    m_defaultOidMap = sd.getDefaultOidMap();
}

void OtaiLinecard::helperLoadColdVids()
{
    SWSS_LOG_ENTER();

    m_coldBootDiscoveredVids = m_client->getColdVids(m_linecard_vid);

    SWSS_LOG_NOTICE("read %zu COLD VIDS", m_coldBootDiscoveredVids.size());
}

std::set<otai_object_id_t> OtaiLinecard::getColdBootDiscoveredVids() const
{
    SWSS_LOG_ENTER();

    if (m_coldBootDiscoveredVids.size() != 0)
    {
        return m_coldBootDiscoveredVids;
    }

    /*
     * Normally we should throw here, but we want to keep backward
     * compatibility and don't break anything.
     */

    SWSS_LOG_WARN("cold boot discovered VIDs set is empty, using discovered set");

    std::set<otai_object_id_t> discoveredVids;

    for (otai_object_id_t rid: m_discovered_rids)
    {
        otai_object_id_t vid = m_translator->translateRidToVid(rid, m_linecard_vid);

        discoveredVids.insert(vid);
    }

    return discoveredVids;
}

std::set<otai_object_id_t> OtaiLinecard::getWarmBootDiscoveredVids() const
{
    SWSS_LOG_ENTER();

    return m_warmBootDiscoveredVids;
}

void OtaiLinecard::redisSaveInternalOids(
        _In_ otai_object_id_t rid) const
{
    SWSS_LOG_ENTER();

    std::set<otai_object_id_t> coldVids;

    otai_object_id_t vid = m_translator->translateRidToVid(rid, m_linecard_vid);

    coldVids.insert(vid);

    /* Save Linecard Internal OID put in current view asic state and also
     * in ColdVid Table discovered as cold or warm boot.
     * Please note it is possible to discover new Linecard internal OID in warm-boot also
     * if OTAI gets upgraded as part of warm-boot so we are adding to ColdVid also
     * so that comparison logic do not remove this OID in warm-boot case. One example
     * is OTAI_LINECARD_ATTR_DEFAULT_STP_INST_ID which is discovered in warm-boot 
     * when upgrading to new OTAI Version*/

    m_client->setDummyAsicStateObject(vid);

    m_client->saveColdBootDiscoveredVids(m_linecard_vid, coldVids);

    SWSS_LOG_NOTICE("put linecard internal discovered rid %s to Asic View and COLDVIDS", 
            otai_serialize_object_id(rid).c_str());

}

void OtaiLinecard::redisSaveColdBootDiscoveredVids() const
{
    SWSS_LOG_ENTER();

    std::set<otai_object_id_t> coldVids;

    for (otai_object_id_t rid: m_discovered_rids)
    {
        otai_object_id_t vid = m_translator->translateRidToVid(rid, m_linecard_vid);

        coldVids.insert(vid);
    }

    m_client->saveColdBootDiscoveredVids(m_linecard_vid, coldVids);
}

void OtaiLinecard::helperSaveDiscoveredObjectsToRedis()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_TIMER("save discovered objects to redis");

    /*
     * There is a problem:
     *
     * After linecard creation, on the linecard objects are created internally like
     * VLAN members, queues, SGs etc.  Some of those objects are removable.
     * User can decide that he don't want VLAN members and he will remove them.
     * Those objects will be removed from ASIC view in redis as well.
     *
     * Now after hard reinit, syncd will pick up what is in the db and it will
     * try to recreate ASIC state.  First it will create linecard, and this
     * linecard will create those VLAN members again inside ASIC and it will try
     * to put them back to the DB, since we need to keep track of all default
     * objects.
     *
     * We need a way to decide whether we need to put those objects to DB or
     * not. Since we are performing syncd hard reinit and recreating linecard
     * that there was something in the DB already. And basing on that we can
     * deduce that we don't need to put again all our discovered objects to the
     * DB, since some of those objects could be removed at the beginning.
     *
     * Hard reinit is performed before taking any action from the redis queue.
     * But when user will decide to create linecard, table consumer will put that
     * linecard to the DB right away before calling OtaiLinecard constructor.  But
     * that linecard will be the only object in the ASIC table, so out simple
     * deduction here could be checking if there is more than object in the db.
     * If there are at least 2, then we don't need to save discovered oids.
     *
     * We will use at least number 32, since that should be at least number of
     * ports that previously discovered. We could also query for PORTs.  We can
     * also deduce this using HIDDEN key objects, if they are defined then that
     * would mean we already put objects to db at the first place.
     *
     * PS. This is not the best way to solve this problem, but works.
     *
     * TODO: Some of those objects could be removed, like vlan members etc, we
     * could actually put those objects back, but only those objects which we
     * would consider non removable, and this is hard to determine now. A
     * method getNonRemovableObjects would be nice, then we could put those
     * objects to the view every time, and not put only discovered objects that
     * reflect removable objects like vlan member.
     *
     * This must be per linecard.
     */

    const int ObjectsTreshold = 32;

    size_t keys = m_client->getAsicObjectsSize(m_linecard_vid);

    bool manyObjectsPresent = keys > ObjectsTreshold;

    SWSS_LOG_NOTICE("objects in ASIC state table present: %zu", keys);

    if (manyObjectsPresent)
    {
        SWSS_LOG_NOTICE("will NOT put discovered objects into db");

        // TODO or just check here and remove directly form ASIC_VIEW ?
        return;
    }

    SWSS_LOG_NOTICE("putting ALL discovered objects to redis");

    for (otai_object_id_t rid: m_discovered_rids)
    {
        /*
         * We also could thing of optimizing this since it's one call to redis
         * per rid, and probably this should be ATOMIC.
         *
         * NOTE: We are also storing read only object's here, like default
         * virtual router, CPU, default trap group, etc.
         */

        redisSetDummyAsicStateForRealObjectId(rid);
    }

    /*
     * If we are here, this is probably COLD boot, since any previous boot
     * would put lots of objects into redis DB (ports, queues, scheduler_groups
     * etc), and since this is cold boot, we can put those discovered objects
     * to cold boot objects map to redis DB. This will become handy when doing
     * warm boot and figuring out which object is default created and which is
     * user created, since after warm boot user could previously assign buffer
     * profile on ingress priority group and this buffer profile will be
     * discovered by otai discovery logic.
     *
     * Question is here whether we should put VID here or RID. And after cold
     * boot when hard reinit logic happens, we need to remap them, also note
     * that some object could be removed like VLAN members and they will not
     * have existing corresponding OID.
     */

    redisSaveColdBootDiscoveredVids();
}

void OtaiLinecard::helperInternalOids()
{
    SWSS_LOG_ENTER();

    auto info = otai_metadata_get_object_type_info(OTAI_OBJECT_TYPE_LINECARD);

    for (int idx = 0; info->attrmetadata[idx] != NULL; ++idx)
    {
        const otai_attr_metadata_t *md = info->attrmetadata[idx];

        if (md->attrvaluetype == OTAI_ATTR_VALUE_TYPE_OBJECT_ID &&
                md->defaultvaluetype == OTAI_DEFAULT_VALUE_TYPE_LINECARD_INTERNAL)
        {
            helperGetLinecardAttrOid(md->attrid);
        }
    }
}

otai_object_id_t OtaiLinecard::getDefaultValueForOidAttr(
        _In_ otai_object_id_t rid,
        _In_ otai_attr_id_t attr_id)
{
    SWSS_LOG_ENTER();

    auto it = m_defaultOidMap.find(rid);

    if (it == m_defaultOidMap.end())
    {
        return OTAI_NULL_OBJECT_ID;
    }

    auto ita = it->second.find(attr_id);

    if (ita == it->second.end())
    {
        return OTAI_NULL_OBJECT_ID;
    }

    return ita->second;
}

void OtaiLinecard::helperPopulateWarmBootVids()
{
    SWSS_LOG_ENTER();

    if (!m_warmBoot)
        return;

    SWSS_LOG_NOTICE("populate warm boot VIDs");

    // It may happen, that after warm boot some new oids were discovered that
    // were not present on warm shutdown, this may happen during vendor OTAI
    // update and for example introducing some new default objects on linecard or
    // queues on cpu. In this case, translator will create new VID/RID pair on
    // database and local memory.

    auto rid2vid = getRidToVidMap();

    for (otai_object_id_t rid: m_discovered_rids)
    {
        otai_object_id_t vid = m_translator->translateRidToVid(rid, m_linecard_vid);

        m_warmBootDiscoveredVids.insert(vid);

        if (rid2vid.find(rid) == rid2vid.end())
        {
            SWSS_LOG_NOTICE("spotted new RID %s (VID %s) on WARM BOOT",
                    otai_serialize_object_id(rid).c_str(),
                    otai_serialize_object_id(vid).c_str());

            m_warmBootNewDiscoveredVids.insert(vid);

            // this means that some new objects were discovered but they are
            // not present in current ASIC_VIEW, and we need to create dummy
            // entries for them

            redisSetDummyAsicStateForRealObjectId(rid);
        }
    }
}

bool OtaiLinecard::isWarmBoot() const
{
    SWSS_LOG_ENTER();

    return m_warmBoot;
}

void OtaiLinecard::checkWarmBootDiscoveredRids()
{
    SWSS_LOG_ENTER();

    /*
     * After linecard was created, rid discovery method was called, and all
     * discovered RIDs should be present in current RID2VID map in redis
     * database. If any RID is missing, then ether there is bug in vendor code,
     * and after warm boot some RID values changed or we have a bug and forgot
     * to put rid/vid pair to redis.
     *
     * Assumption here is that during warm boot ASIC state will not change.
     */

    auto rid2vid = getRidToVidMap();

    bool success = true;

    for (auto rid: getDiscoveredRids())
    {
        if (rid2vid.find(rid) != rid2vid.end())
            continue;

        SWSS_LOG_ERROR("RID %s is missing from current RID2VID map after WARM boot!",
                otai_serialize_object_id(rid).c_str());

        success = false;
    }

    if (!success)
    {
        SWSS_LOG_THROW("FATAL, some discovered RIDs are not present in current RID2VID map, bug");
    }

    SWSS_LOG_NOTICE("all discovered RIDs are present in current RID2VID map for linecard VID %s",
            otai_serialize_object_id(m_linecard_vid).c_str());
}

