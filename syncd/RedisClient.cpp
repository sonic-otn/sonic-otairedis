#include "RedisClient.h"
#include "VidManager.h"

#include "lairediscommon.h"

#include "meta/lai_serialize.h"

#include "swss/logger.h"
#include "swss/redisapi.h"

using namespace syncd;

#define VIDTORID                    "VIDTORID"
#define RIDTOVID                    "RIDTOVID"
#define LANES                       "LANES"
#define HIDDEN                      "HIDDEN"
#define COLDVIDS                    "COLDVIDS"

RedisClient::RedisClient(
        _In_ std::shared_ptr<swss::DBConnector> dbAsic,
        _In_ std::shared_ptr<swss::DBConnector> dbFlexCounter):
    m_dbAsic(dbAsic),
    m_dbFlexcounter(dbFlexCounter)
{
    SWSS_LOG_ENTER();
}

RedisClient::~RedisClient()
{
    SWSS_LOG_ENTER();

    // empty
}

std::string RedisClient::getRedisLanesKey(
        _In_ lai_object_id_t linecardVid) const
{
    SWSS_LOG_ENTER();

    /*
     * Each linecard will have it's own lanes: LANES:oid:0xYYYYYYYY.
     *
     * NOTE: To support multiple linecards LANES needs to be made per linecard.
     *
     * return std::string(LANES) + ":" + lai_serialize_object_id(m_linecard_vid);
     *
     * Only linecard with index 0 and global context 0 will have key "LANES" for
     * backward compatibility. We could convert that during runtime at first
     * time.
     */

    auto index = VidManager::getLinecardIndex(linecardVid);

    auto context = VidManager::getGlobalContext(linecardVid);

    if (index == 0 && context == 0)
    {
        return std::string(LANES);
    }

    return (LANES ":") + lai_serialize_object_id(linecardVid);
}


void RedisClient::clearLaneMap(
        _In_ lai_object_id_t linecardVid) const
{
    SWSS_LOG_ENTER();

    auto key = getRedisLanesKey(linecardVid);

    m_dbAsic->del(key);
}

std::unordered_map<lai_uint32_t, lai_object_id_t> RedisClient::getLaneMap(
        _In_ lai_object_id_t linecardVid) const
{
    SWSS_LOG_ENTER();

    auto key = getRedisLanesKey(linecardVid);

    auto hash = m_dbAsic->hgetall(key);

    SWSS_LOG_DEBUG("previous lanes: %zu", hash.size());

    std::unordered_map<lai_uint32_t, lai_object_id_t> map;

    for (auto &kv: hash)
    {
        const std::string &str_key = kv.first;
        const std::string &str_value = kv.second;

        lai_uint32_t lane;
        lai_object_id_t portId;

        lai_deserialize_number(str_key, lane);

        lai_deserialize_object_id(str_value, portId);

        map[lane] = portId;
    }

    return map;
}

void RedisClient::saveLaneMap(
        _In_ lai_object_id_t linecardVid,
        _In_ const std::unordered_map<lai_uint32_t, lai_object_id_t>& map) const
{
    SWSS_LOG_ENTER();

    clearLaneMap(linecardVid);

    for (auto const &it: map)
    {
        lai_uint32_t lane = it.first;
        lai_object_id_t portId = it.second;

        std::string strLane = lai_serialize_number(lane);
        std::string strPortId = lai_serialize_object_id(portId);

        auto key = getRedisLanesKey(linecardVid);

        m_dbAsic->hset(key, strLane, strPortId);
    }
}

std::unordered_map<lai_object_id_t, lai_object_id_t> RedisClient::getObjectMap(
        _In_ const std::string &key) const
{
    SWSS_LOG_ENTER();

    auto hash = m_dbAsic->hgetall(key);

    std::unordered_map<lai_object_id_t, lai_object_id_t> map;

    for (auto &kv: hash)
    {
        const std::string &str_key = kv.first;
        const std::string &str_value = kv.second;

        lai_object_id_t objectIdKey;
        lai_object_id_t objectIdValue;

        lai_deserialize_object_id(str_key, objectIdKey);

        lai_deserialize_object_id(str_value, objectIdValue);

        map[objectIdKey] = objectIdValue;
    }

    return map;
}

std::unordered_map<lai_object_id_t, lai_object_id_t> RedisClient::getVidToRidMap(
        _In_ lai_object_id_t linecardVid) const
{
    SWSS_LOG_ENTER();

    auto map = getObjectMap(VIDTORID);

    std::unordered_map<lai_object_id_t, lai_object_id_t> filtered;

    for (auto& v2r: map)
    {
        auto linecardId = VidManager::linecardIdQuery(v2r.first);

        if (linecardId == linecardVid)
        {
            filtered[v2r.first] = v2r.second;
        }
    }

    return filtered;
}

std::unordered_map<lai_object_id_t, lai_object_id_t> RedisClient::getRidToVidMap(
        _In_ lai_object_id_t linecardVid) const
{
    SWSS_LOG_ENTER();

    auto map = getObjectMap(RIDTOVID);

    std::unordered_map<lai_object_id_t, lai_object_id_t> filtered;

    for (auto& r2v: map)
    {
        auto linecardId = VidManager::linecardIdQuery(r2v.second);

        if (linecardId == linecardVid)
        {
            filtered[r2v.first] = r2v.second;
        }
    }

    return filtered;
}

std::unordered_map<lai_object_id_t, lai_object_id_t> RedisClient::getVidToRidMap() const
{
    SWSS_LOG_ENTER();

    return getObjectMap(VIDTORID);
}

std::unordered_map<lai_object_id_t, lai_object_id_t> RedisClient::getRidToVidMap() const
{
    SWSS_LOG_ENTER();

    return getObjectMap(RIDTOVID);
}

void RedisClient::setDummyAsicStateObject(
        _In_ lai_object_id_t objectVid)
{
    SWSS_LOG_ENTER();

    lai_object_type_t objectType = VidManager::objectTypeQuery(objectVid);

    std::string strObjectType = lai_serialize_object_type(objectType);

    std::string strVid = lai_serialize_object_id(objectVid);

    std::string strKey = ASIC_STATE_TABLE + (":" + strObjectType + ":" + strVid);

    m_dbAsic->hset(strKey, "NULL", "NULL");
}

std::string RedisClient::getRedisColdVidsKey(
        _In_ lai_object_id_t linecardVid) const
{
    SWSS_LOG_ENTER();

    /*
     * Each linecard will have it's own cold vids: COLDVIDS:oid:0xYYYYYYYY.
     *
     * NOTE: To support multiple linecards COLDVIDS needs to be made per linecard.
     *
     * return std::string(COLDVIDS) + ":" + lai_serialize_object_id(m_linecard_vid);
     *
     * Only linecard with index 0 and global context 0 will have key "COLDVIDS" for
     * backward compatibility. We could convert that during runtime at first
     * time.
     */

    auto index = VidManager::getLinecardIndex(linecardVid);

    auto context = VidManager::getGlobalContext(linecardVid);

    if (index == 0 && context == 0)
    {
        return std::string(COLDVIDS);
    }

    return (COLDVIDS ":") + lai_serialize_object_id(linecardVid);
}

void RedisClient::saveColdBootDiscoveredVids(
        _In_ lai_object_id_t linecardVid,
        _In_ const std::set<lai_object_id_t>& coldVids)
{
    SWSS_LOG_ENTER();

    auto key = getRedisColdVidsKey(linecardVid);

    for (auto vid: coldVids)
    {
        lai_object_type_t objectType = VidManager::objectTypeQuery(vid);

        std::string strObjectType = lai_serialize_object_type(objectType);

        std::string strVid = lai_serialize_object_id(vid);

        m_dbAsic->hset(key, strVid, strObjectType);
    }
}

std::string RedisClient::getRedisHiddenKey(
        _In_ lai_object_id_t linecardVid) const
{
    SWSS_LOG_ENTER();

    /*
     * Each linecard will have it's own hidden: HIDDEN:oid:0xYYYYYYYY.
     *
     * NOTE: To support multiple linecards HIDDEN needs to be made per linecard.
     *
     * return std::string(HIDDEN) + ":" + lai_serialize_object_id(m_linecard_vid);
     *
     * Only linecard with index 0 and global context 0 will have key "HIDDEN" for
     * backward compatibility. We could convert that during runtime at first
     * time.
     */

    auto index = VidManager::getLinecardIndex(linecardVid);

    auto context = VidManager::getGlobalContext(linecardVid);

    if (index == 0 && context == 0)
    {
        return std::string(HIDDEN);
    }

    return (HIDDEN ":") + lai_serialize_object_id(linecardVid);
}

std::shared_ptr<std::string> RedisClient::getLinecardHiddenAttribute(
        _In_ lai_object_id_t linecardVid,
        _In_ const std::string& attrIdName)
{
    SWSS_LOG_ENTER();

    auto key = getRedisHiddenKey(linecardVid);

    return m_dbAsic->hget(key, attrIdName);
}

void RedisClient::saveLinecardHiddenAttribute(
        _In_ lai_object_id_t linecardVid,
        _In_ const std::string& attrIdName,
        _In_ lai_object_id_t objectRid)
{
    SWSS_LOG_ENTER();

    auto key = getRedisHiddenKey(linecardVid);

    std::string strRid = lai_serialize_object_id(objectRid);

    m_dbAsic->hset(key, attrIdName, strRid);
}

std::set<lai_object_id_t> RedisClient::getColdVids(
        _In_ lai_object_id_t linecardVid)
{
    SWSS_LOG_ENTER();

    auto key = getRedisColdVidsKey(linecardVid);

    auto hash = m_dbAsic->hgetall(key);

    /*
     * NOTE: some objects may not exists after 2nd restart, like VLAN_MEMBER or
     * BRIDGE_PORT, since user could decide to remove them on previous boot.
     */

    std::set<lai_object_id_t> coldVids;

    for (auto kvp: hash)
    {
        auto strVid = kvp.first;

        lai_object_id_t vid;
        lai_deserialize_object_id(strVid, vid);

        /*
         * Just make sure that vid in COLDVIDS is present in current vid2rid map
         */

        auto rid = m_dbAsic->hget(VIDTORID, strVid);

        if (rid == nullptr)
        {
            SWSS_LOG_INFO("no RID for VID %s, probably object was removed previously", strVid.c_str());
        }

        coldVids.insert(vid);
    }

    return coldVids;
}

void RedisClient::setPortLanes(
        _In_ lai_object_id_t linecardVid,
        _In_ lai_object_id_t portRid,
        _In_ const std::vector<uint32_t>& lanes)
{
    SWSS_LOG_ENTER();

    auto key = getRedisLanesKey(linecardVid);

    for (uint32_t lane: lanes)
    {
        std::string strLane = lai_serialize_number(lane);
        std::string strPortRid = lai_serialize_object_id(portRid);

        m_dbAsic->hset(key, strLane, strPortRid);
    }
}

size_t RedisClient::getAsicObjectsSize(
        _In_ lai_object_id_t linecardVid) const
{
    SWSS_LOG_ENTER();

    // NOTE: this goes over all objects, and if we have N linecards then it will
    // go N times on every linecard and it can be slow, we need to find better
    // way to do this

    auto keys = m_dbAsic->keys(ASIC_STATE_TABLE ":*");

    size_t count = 0;

    for (auto& key: keys)
    {
        auto mk = key.substr(key.find_first_of(":") + 1);

        lai_object_meta_key_t metaKey;

        lai_deserialize_object_meta_key(mk, metaKey);

        // we need to check only objects that's belong to requested linecard

        auto swid = VidManager::linecardIdQuery(metaKey.objectkey.key.object_id);

        if (swid == linecardVid)
        {
            count++;
        }
    }

    return count;
}

int RedisClient::removePortFromLanesMap(
        _In_ lai_object_id_t linecardVid,
        _In_ lai_object_id_t portRid) const
{
    SWSS_LOG_ENTER();

    // key - lane number, value - port RID
    auto map = getLaneMap(linecardVid);

    int removed = 0;

    auto key = getRedisLanesKey(linecardVid);

    for (auto& kv: map)
    {
        if (kv.second == portRid)
        {
            std::string strLane = lai_serialize_number(kv.first);

            m_dbAsic->hdel(key, strLane);

            removed++;
        }
    }

    return removed;
}

void RedisClient::removeAsicObject(
        _In_ lai_object_id_t objectVid) const
{
    SWSS_LOG_ENTER();

    lai_object_type_t ot = VidManager::objectTypeQuery(objectVid);

    auto strVid = lai_serialize_object_id(objectVid);

    std::string key = (ASIC_STATE_TABLE ":") + lai_serialize_object_type(ot) + ":" + strVid;

    SWSS_LOG_INFO("removing ASIC DB key: %s", key.c_str());

    m_dbAsic->del(key);
}

void RedisClient::removeAsicObject(
        _In_ const lai_object_meta_key_t& metaKey)
{
    SWSS_LOG_ENTER();

    std::string key = (ASIC_STATE_TABLE ":") + lai_serialize_object_meta_key(metaKey);

    m_dbAsic->del(key);
}

void RedisClient::removeAsicObjects(
        _In_ const std::vector<std::string>& keys)
{
    SWSS_LOG_ENTER();

    std::vector<std::string> prefixKeys;

    // we need to rewrite keys to add table prefix
    for (const auto& key: keys)
    {
         prefixKeys.push_back((ASIC_STATE_TABLE ":") + key);
    }

    m_dbAsic->del(prefixKeys);
}

void RedisClient::setAsicObject(
        _In_ const lai_object_meta_key_t& metaKey,
        _In_ const std::string& attr,
        _In_ const std::string& value)
{
    SWSS_LOG_ENTER();

    std::string key = (ASIC_STATE_TABLE ":") + lai_serialize_object_meta_key(metaKey);

    m_dbAsic->hset(key, attr, value);
}

void RedisClient::createAsicObject(
        _In_ const lai_object_meta_key_t& metaKey,
        _In_ const std::vector<swss::FieldValueTuple>& attrs)
{
    SWSS_LOG_ENTER();

    std::string key = (ASIC_STATE_TABLE ":") + lai_serialize_object_meta_key(metaKey);

    if (attrs.size() == 0)
    {
        m_dbAsic->hset(key, "NULL", "NULL");
        return;
    }

    for (const auto& e: attrs)
    {
        m_dbAsic->hset(key, fvField(e), fvValue(e));
    }
}

void RedisClient::createAsicObjects(
        _In_ const std::unordered_map<std::string, std::vector<swss::FieldValueTuple>>& multiHash)
{
    SWSS_LOG_ENTER();

    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> hash;

    // we need to rewrite hash to add table prefix
    for (const auto& kvp: multiHash)
    {
        hash[(ASIC_STATE_TABLE ":") + kvp.first] = kvp.second;

        if (kvp.second.size() == 0)
        {
            hash[(ASIC_STATE_TABLE ":") + kvp.first].emplace_back(std::make_pair<std::string, std::string>("NULL", "NULL"));
        }
    }

    m_dbAsic->hmset(hash);
}

void RedisClient::setVidAndRidMap(
        _In_ const std::unordered_map<lai_object_id_t, lai_object_id_t>& map)
{
    SWSS_LOG_ENTER();

    m_dbAsic->del(VIDTORID);
    m_dbAsic->del(RIDTOVID);

    for (auto &kv: map)
    {
        std::string strVid = lai_serialize_object_id(kv.first);
        std::string strRid = lai_serialize_object_id(kv.second);

        m_dbAsic->hset(VIDTORID, strVid, strRid);
        m_dbAsic->hset(RIDTOVID, strRid, strVid);
    }
}

std::vector<std::string> RedisClient::getAsicStateKeys() const
{
    SWSS_LOG_ENTER();

    return m_dbAsic->keys(ASIC_STATE_TABLE ":*");
}

std::vector<std::string> RedisClient::getFlexCounterKeys() const
{
    SWSS_LOG_ENTER();

    return m_dbFlexcounter->keys(FLEX_COUNTER_TABLE ":*");
}

std::vector<std::string> RedisClient::getFlexCounterGroupKeys() const
{
    SWSS_LOG_ENTER();

    return m_dbFlexcounter->keys(FLEX_COUNTER_GROUP_TABLE ":*");
}

std::vector<std::string> RedisClient::getAsicStateLinecardsKeys() const
{
    SWSS_LOG_ENTER();

    return m_dbAsic->keys(ASIC_STATE_TABLE ":LAI_OBJECT_TYPE_LINECARD:*");
}

void RedisClient::removeColdVid(
        _In_ lai_object_id_t vid)
{
    SWSS_LOG_ENTER();

    auto strVid = lai_serialize_object_id(vid);

    m_dbAsic->hdel(COLDVIDS, strVid);
}

std::unordered_map<std::string, std::string> RedisClient::getAttributesFromAsicKey(
        _In_ const std::string& key) const
{
    SWSS_LOG_ENTER();

    std::unordered_map<std::string, std::string> map;
    m_dbAsic->hgetall(key, std::inserter(map, map.end()));
    return map;
}

std::unordered_map<std::string, std::string> RedisClient::getAttributesFromFlexCounterGroupKey(
        _In_ const std::string& key) const
{
    SWSS_LOG_ENTER();

    std::unordered_map<std::string, std::string> map;
    m_dbFlexcounter->hgetall(key, std::inserter(map, map.end()));
    return map;
}

std::unordered_map<std::string, std::string> RedisClient::getAttributesFromFlexCounterKey(
        _In_ const std::string& key) const
{
    SWSS_LOG_ENTER();

    std::unordered_map<std::string, std::string> map;
    m_dbFlexcounter->hgetall(key, std::inserter(map, map.end()));
    return map;
}

bool RedisClient::hasNoHiddenKeysDefined() const
{
    SWSS_LOG_ENTER();

    auto keys = m_dbAsic->keys(HIDDEN "*");

    return keys.size() == 0;
}

void RedisClient::removeVidAndRid(
        _In_ lai_object_id_t vid,
        _In_ lai_object_id_t rid)
{
    SWSS_LOG_ENTER();

    auto strVid = lai_serialize_object_id(vid);
    auto strRid = lai_serialize_object_id(rid);

    m_dbAsic->hdel(VIDTORID, strVid);
    m_dbAsic->hdel(RIDTOVID, strRid);
}

void RedisClient::insertVidAndRid(
        _In_ lai_object_id_t vid,
        _In_ lai_object_id_t rid)
{
    SWSS_LOG_ENTER();

    auto strVid = lai_serialize_object_id(vid);
    auto strRid = lai_serialize_object_id(rid);

    m_dbAsic->hset(VIDTORID, strVid, strRid);
    m_dbAsic->hset(RIDTOVID, strRid, strVid);
}

lai_object_id_t RedisClient::getVidForRid(
        _In_ lai_object_id_t rid)
{
    SWSS_LOG_ENTER();

    auto strRid = lai_serialize_object_id(rid);

    auto pvid = m_dbAsic->hget(RIDTOVID, strRid);

    if (pvid == nullptr)
    {
        // rid2vid map should never contain null, so we can return NULL which
        // will mean that mapping don't exists

        return LAI_NULL_OBJECT_ID;
    }

    lai_object_id_t vid;

    lai_deserialize_object_id(*pvid, vid);

    return vid;
}

lai_object_id_t RedisClient::getRidForVid(
        _In_ lai_object_id_t vid)
{
    SWSS_LOG_ENTER();

    auto strVid = lai_serialize_object_id(vid);

    auto prid = m_dbAsic->hget(VIDTORID, strVid);

    if (prid == nullptr)
    {
        // rid2vid map should never contain null, so we can return NULL which
        // will mean that mapping don't exists

        return LAI_NULL_OBJECT_ID;
    }

    lai_object_id_t rid;

    lai_deserialize_object_id(*prid, rid);

    return rid;
}

void RedisClient::removeAsicStateTable()
{
    SWSS_LOG_ENTER();

    const auto &asicStateKeys = m_dbAsic->keys(ASIC_STATE_TABLE ":*");

    for (const auto &key: asicStateKeys)
    {
        m_dbAsic->del(key);
    }
}

