#include "FlexCounterReiniter.h"
#include "VidManager.h"
#include "CommandLineOptions.h"
#include "NotificationHandler.h"
#include "RedisClient.h"

#include "swss/logger.h"

#include "meta/otai_serialize.h"

#include <unistd.h>
#include <inttypes.h>

using namespace syncd;
using namespace otaimeta;

FlexCounterReiniter::FlexCounterReiniter(
    _In_ std::shared_ptr<RedisClient> client,
    _In_ std::shared_ptr<VirtualOidTranslator> translator,
    _In_ std::shared_ptr<FlexCounterManager> manager,
    _In_ const ObjectIdMap& vidToRidMap,
    _In_ const ObjectIdMap& ridToVidMap,
    _In_ const std::vector<std::string>& flexCounterKeys) :
    m_client(client),
    m_translator(translator),
    m_manager(manager),
    m_vidToRidMap(vidToRidMap),
    m_ridToVidMap(ridToVidMap),
    m_flexCounterKeys(flexCounterKeys)

{
    SWSS_LOG_ENTER();

    m_linecard_rid = OTAI_NULL_OBJECT_ID;
    m_linecard_vid = OTAI_NULL_OBJECT_ID;
}

FlexCounterReiniter::~FlexCounterReiniter()
{
    SWSS_LOG_ENTER();

    // empty
}

void FlexCounterReiniter::getInfoFromFlexCounterKey(
    _In_ const std::string& key, _Out_ std::string& instancdID, _Out_ otai_object_id_t& vid, _Out_ otai_object_id_t& rid)
{
    SWSS_LOG_ENTER();

    auto start = key.find_first_of(":") + 1;
    auto end = key.find(":", start);

    const std::string strObjectType = key.substr(start, end - start);
    instancdID = strObjectType;
    std::string strObjectId = key.substr(end + 1);

    otai_deserialize_object_id(strObjectId, vid);

    if (!m_translator->tryTranslateVidToRid(vid, rid))
    {
        SWSS_LOG_WARN("port VID %s, was not found (probably port was removed/splitted) and will remove from counters now",
            otai_serialize_object_id(vid).c_str());
    }

    SWSS_LOG_NOTICE("FlexCounterReiniter getObjectTypeFromFlexCounterKey strObjectType is %s rid is 0x%" PRIx64 " vid is 0x%" PRIx64 "",
        strObjectType.c_str(), rid, vid);

    return;
}

void FlexCounterReiniter::prepareFlexCounterState()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_TIMER("read flexCounter state");
    for (auto& key : m_flexCounterKeys)
    {
        otai_object_id_t rid;
        otai_object_id_t vid;
        std::string instancdID;
        m_values = redisGetAttributesFromKey(key);
        getInfoFromFlexCounterKey(key, instancdID, vid, rid);
        SWSS_LOG_NOTICE("key is %s vid is 0x%" PRIx64 " rid is 0x%" PRIx64 " instanceID is %s", key.c_str(), vid, rid, instancdID.c_str());
        m_manager->addCounter(vid, rid, instancdID, m_values);
    }
}

std::vector<swss::FieldValueTuple> FlexCounterReiniter::redisGetAttributesFromKey(
    _In_ const std::string& key)
{
    SWSS_LOG_ENTER();

    std::vector<swss::FieldValueTuple> values;

    auto hash = m_client->getAttributesFromFlexCounterKey(key);

    for (auto& kv : hash)
    {
        const std::string& skey = kv.first;
        const std::string& svalue = kv.second;
        SWSS_LOG_DEBUG("redisGetAttributesFromKey skey is %s svalue is %s", skey.c_str(), svalue.c_str());
        swss::FieldValueTuple fvt(skey, svalue);

        values.push_back(fvt);
    }

    return values;
}

void FlexCounterReiniter::hardReinit()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_TIMER("FlexCounterReiniter hard reinit");
    prepareFlexCounterState();
    return;
}



FlexCounterGroupReiniter::FlexCounterGroupReiniter(
    _In_ std::shared_ptr<RedisClient> client,
    _In_ std::shared_ptr<FlexCounterManager> manager,
    _In_ const std::vector<std::string>& flexCounterGroupKeys) :
    m_client(client),
    m_manager(manager),
    m_flexCounterGroupKeys(flexCounterGroupKeys)
{
    SWSS_LOG_ENTER();
}

FlexCounterGroupReiniter::~FlexCounterGroupReiniter()
{
    SWSS_LOG_ENTER();

    // empty
}

void FlexCounterGroupReiniter::prepareFlexCounterGroupState()
{
    SWSS_LOG_ENTER();
    std::string groupName;
    SWSS_LOG_TIMER("read flexCounter group state");
    for (auto& key : m_flexCounterGroupKeys)
    {
        m_values = redisGetAttributesFromKey(key);
        groupName = getGroupNameFromGroupKey(key);
        SWSS_LOG_NOTICE("prepareFlexCounterGroupState key is %s groupName is %s value ", groupName.c_str(), key.c_str());
        m_manager->addCounterPlugin(groupName, m_values);
    }
}

void FlexCounterGroupReiniter::hardReinit()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_TIMER("FlexCounterGroupReiniter hard reinit");
    prepareFlexCounterGroupState();

    return;
}

std::vector<swss::FieldValueTuple> FlexCounterGroupReiniter::redisGetAttributesFromKey(
    _In_ const std::string& key)
{
    SWSS_LOG_ENTER();

    std::vector<swss::FieldValueTuple> values;

    auto hash = m_client->getAttributesFromFlexCounterGroupKey(key);

    for (auto& kv : hash)
    {
        const std::string& skey = kv.first;
        const std::string& svalue = kv.second;
        SWSS_LOG_DEBUG("redisGetAttributesFromKey skey is %s svalue is %s", skey.c_str(), svalue.c_str());
        swss::FieldValueTuple fvt(skey, svalue);

        values.push_back(fvt);
    }

    return values;
}

std::string  FlexCounterGroupReiniter::getGroupNameFromGroupKey(
    _In_ const std::string& key)
{
    SWSS_LOG_ENTER();
    auto start = key.find_first_of(":") + 1;
    const std::string strObjectType = key.substr(start);
    SWSS_LOG_DEBUG("getGroupNameFromGroupKey skey key is %s strObjectType %s", key.c_str(), strObjectType.c_str());
    return strObjectType;
}

