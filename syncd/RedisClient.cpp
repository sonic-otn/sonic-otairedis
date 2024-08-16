#include "RedisClient.h"
#include "VidManager.h"

#include "otairediscommon.h"

#include "meta/otai_serialize.h"

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

std::unordered_map<otai_object_id_t, otai_object_id_t> RedisClient::getObjectMap(
        _In_ const std::string &key) const
{
    SWSS_LOG_ENTER();

    auto hash = m_dbAsic->hgetall(key);

    std::unordered_map<otai_object_id_t, otai_object_id_t> map;

    for (auto &kv: hash)
    {
        const std::string &str_key = kv.first;
        const std::string &str_value = kv.second;

        otai_object_id_t objectIdKey;
        otai_object_id_t objectIdValue;

        otai_deserialize_object_id(str_key, objectIdKey);

        otai_deserialize_object_id(str_value, objectIdValue);

        map[objectIdKey] = objectIdValue;
    }

    return map;
}

std::unordered_map<otai_object_id_t, otai_object_id_t> RedisClient::getVidToRidMap(
        _In_ otai_object_id_t linecardVid) const
{
    SWSS_LOG_ENTER();

    auto map = getObjectMap(VIDTORID);

    std::unordered_map<otai_object_id_t, otai_object_id_t> filtered;

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

std::unordered_map<otai_object_id_t, otai_object_id_t> RedisClient::getRidToVidMap(
        _In_ otai_object_id_t linecardVid) const
{
    SWSS_LOG_ENTER();

    auto map = getObjectMap(RIDTOVID);

    std::unordered_map<otai_object_id_t, otai_object_id_t> filtered;

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

std::unordered_map<otai_object_id_t, otai_object_id_t> RedisClient::getVidToRidMap() const
{
    SWSS_LOG_ENTER();

    return getObjectMap(VIDTORID);
}

std::unordered_map<otai_object_id_t, otai_object_id_t> RedisClient::getRidToVidMap() const
{
    SWSS_LOG_ENTER();

    return getObjectMap(RIDTOVID);
}

void RedisClient::removeAsicObject(
        _In_ otai_object_id_t objectVid) const
{
    SWSS_LOG_ENTER();

    otai_object_type_t ot = VidManager::objectTypeQuery(objectVid);

    auto strVid = otai_serialize_object_id(objectVid);

    std::string key = (ASIC_STATE_TABLE ":") + otai_serialize_object_type(ot) + ":" + strVid;

    SWSS_LOG_INFO("removing ASIC DB key: %s", key.c_str());

    m_dbAsic->del(key);
}

void RedisClient::removeAsicObject(
        _In_ const otai_object_meta_key_t& metaKey)
{
    SWSS_LOG_ENTER();

    std::string key = (ASIC_STATE_TABLE ":") + otai_serialize_object_meta_key(metaKey);

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
        _In_ const otai_object_meta_key_t& metaKey,
        _In_ const std::string& attr,
        _In_ const std::string& value)
{
    SWSS_LOG_ENTER();

    std::string key = (ASIC_STATE_TABLE ":") + otai_serialize_object_meta_key(metaKey);

    m_dbAsic->hset(key, attr, value);
}

void RedisClient::createAsicObject(
        _In_ const otai_object_meta_key_t& metaKey,
        _In_ const std::vector<swss::FieldValueTuple>& attrs)
{
    SWSS_LOG_ENTER();

    std::string key = (ASIC_STATE_TABLE ":") + otai_serialize_object_meta_key(metaKey);

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
        _In_ const std::unordered_map<otai_object_id_t, otai_object_id_t>& map)
{
    SWSS_LOG_ENTER();

    m_dbAsic->del(VIDTORID);
    m_dbAsic->del(RIDTOVID);

    for (auto &kv: map)
    {
        std::string strVid = otai_serialize_object_id(kv.first);
        std::string strRid = otai_serialize_object_id(kv.second);

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

void RedisClient::removeVidAndRid(
        _In_ otai_object_id_t vid,
        _In_ otai_object_id_t rid)
{
    SWSS_LOG_ENTER();

    auto strVid = otai_serialize_object_id(vid);
    auto strRid = otai_serialize_object_id(rid);

    m_dbAsic->hdel(VIDTORID, strVid);
    m_dbAsic->hdel(RIDTOVID, strRid);
}

void RedisClient::insertVidAndRid(
        _In_ otai_object_id_t vid,
        _In_ otai_object_id_t rid)
{
    SWSS_LOG_ENTER();

    auto strVid = otai_serialize_object_id(vid);
    auto strRid = otai_serialize_object_id(rid);

    m_dbAsic->hset(VIDTORID, strVid, strRid);
    m_dbAsic->hset(RIDTOVID, strRid, strVid);
}

otai_object_id_t RedisClient::getVidForRid(
        _In_ otai_object_id_t rid)
{
    SWSS_LOG_ENTER();

    auto strRid = otai_serialize_object_id(rid);

    auto pvid = m_dbAsic->hget(RIDTOVID, strRid);

    if (pvid == nullptr)
    {
        // rid2vid map should never contain null, so we can return NULL which
        // will mean that mapping don't exists

        return OTAI_NULL_OBJECT_ID;
    }

    otai_object_id_t vid;

    otai_deserialize_object_id(*pvid, vid);

    return vid;
}

otai_object_id_t RedisClient::getRidForVid(
        _In_ otai_object_id_t vid)
{
    SWSS_LOG_ENTER();

    auto strVid = otai_serialize_object_id(vid);

    auto prid = m_dbAsic->hget(VIDTORID, strVid);

    if (prid == nullptr)
    {
        // rid2vid map should never contain null, so we can return NULL which
        // will mean that mapping don't exists

        return OTAI_NULL_OBJECT_ID;
    }

    otai_object_id_t rid;

    otai_deserialize_object_id(*prid, rid);

    return rid;
}

