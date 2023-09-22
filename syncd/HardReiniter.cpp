#include "HardReiniter.h"
#include "VidManager.h"
#include "SingleReiniter.h"
#include "FlexCounterReiniter.h"
#include "RedisClient.h"

#include "swss/logger.h"

#include "meta/otai_serialize.h"

using namespace syncd;

HardReiniter::HardReiniter(
        _In_ std::shared_ptr<RedisClient> client,
        _In_ std::shared_ptr<VirtualOidTranslator> translator,
        _In_ std::shared_ptr<otairedis::OtaiInterface> otai,
        _In_ std::shared_ptr<NotificationHandler> handler,
        _In_ std::shared_ptr<FlexCounterManager> manager):
    m_vendorOtai(otai),
    m_translator(translator),
    m_client(client),
    m_handler(handler),
    m_manager(manager)
{
    SWSS_LOG_ENTER();

    // empty
}

HardReiniter::~HardReiniter()
{
    SWSS_LOG_ENTER();

    // empty
}

void HardReiniter::readAsicState()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_TIMER("read asic state");

    // Repopulate asic view from redis db after hard asic initialize.

    m_vidToRidMap = m_client->getVidToRidMap();
    m_ridToVidMap = m_client->getRidToVidMap();

    for (auto& v2r: m_vidToRidMap)
    {
        auto linecardId = VidManager::linecardIdQuery(v2r.first);

        m_linecardVidToRid[linecardId][v2r.first] = v2r.second;
    }

    for (auto& r2v: m_ridToVidMap)
    {
        auto linecardId = VidManager::linecardIdQuery(r2v.second);

        m_linecardRidToVid[linecardId][r2v.first] = r2v.second;
    }

    auto asicStateKeys = m_client->getAsicStateKeys();

    for (const auto &key: asicStateKeys)
    {
        auto mk = key.substr(key.find_first_of(":") + 1); // skip asic key

        otai_object_meta_key_t metaKey;
        otai_deserialize_object_meta_key(mk, metaKey);

        // if object is non object id then first item will be linecard id

        auto linecardId = VidManager::linecardIdQuery(metaKey.objectkey.key.object_id);

        m_linecardMap[linecardId].push_back(key);
    }
 

    SWSS_LOG_NOTICE("loaded %zu linecards", m_linecardMap.size());

    for (auto& kvp: m_linecardMap)
    {
        SWSS_LOG_NOTICE("linecard VID: %s keys %d", otai_serialize_object_id(kvp.first).c_str(), (int)kvp.second.size());
    }
}

std::map<otai_object_id_t, std::shared_ptr<syncd::OtaiLinecard>> HardReiniter::hardReinit()
{
    SWSS_LOG_ENTER();

    readAsicState();

    auto flexCounterGroupKeys = m_client->getFlexCounterGroupKeys();
    auto flexCounterKeys = m_client->getFlexCounterKeys();
    
    std::vector<std::shared_ptr<SingleReiniter>> vec;

    // perform hard reinit on all linecards

    for (auto& kvp: m_linecardMap)
    {
        auto sr = std::make_shared<SingleReiniter>(
                m_client,
                m_translator,
                m_vendorOtai,
                m_handler,
                m_linecardVidToRid.at(kvp.first),
                m_linecardRidToVid.at(kvp.first),
                kvp.second);

        sr->hardReinit();

        vec.push_back(sr);
    }

    auto fgr = std::make_shared<FlexCounterGroupReiniter>(
                m_client,
                m_manager,
                flexCounterGroupKeys);

    fgr->hardReinit();

    for (auto& kvp: m_linecardMap)
    {
        auto fr = std::make_shared<FlexCounterReiniter>(
                m_client,
                m_translator,
                m_manager,
                m_linecardVidToRid.at(kvp.first),
                m_linecardRidToVid.at(kvp.first),
                flexCounterKeys);
        fr->hardReinit();
    }

    // since vid and rid maps contains all linecards
    // we need to combine them

    ObjectIdMap vid2rid;
    ObjectIdMap rid2vid;

    for (auto&sr: vec)
    {
        auto map = sr->getTranslatedVid2Rid();

        for (auto&kvp: map)
        {
            vid2rid[kvp.first] = kvp.second;
            rid2vid[kvp.second] = kvp.first;
        }
    }

    if (vid2rid.size() != rid2vid.size())
    {
        SWSS_LOG_THROW("FATAL: vid2rid %zu != rid2vid %zu",
                vid2rid.size(),
                rid2vid.size());
    }

    // now some object could be removed from linecard then we need to execute
    // post actions, and since those actions will be executed on linecards which
    // will also modify redis database we need to execute this after we put vid
    // and rid map

    /*
     * NOTE: clear can be done after recreating all linecards unless vid/rid map
     * will be per linecard. An all this must be ATOMIC.
     *
     * This needs to be addressed when we want to support multiple linecards.
     */

    m_client->setVidAndRidMap(vid2rid);

    std::map<otai_object_id_t, std::shared_ptr<syncd::OtaiLinecard>> linecards;

    for (auto& sr: vec)
    {
        sr->postRemoveActions();

        auto sw = sr->getLinecard();

        linecards[sw->getVid()] = sw;
    }

    return linecards;
}
