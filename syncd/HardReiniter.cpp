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

    // Repopulate linecard keys and vid-rid maps from redis db
    m_vidToRidMap = m_client->getVidToRidMap();
    m_ridToVidMap = m_client->getRidToVidMap();
    m_linecardKeys = m_client->getAsicStateKeys();
}

std::shared_ptr<syncd::OtaiLinecard> HardReiniter::hardReinit()
{
    SWSS_LOG_ENTER();

    readAsicState();
    
    // perform reinit on the linecard
    auto sr = std::make_shared<SingleReiniter>(
            m_client,
            m_translator,
            m_vendorOtai,
            m_handler,
            m_vidToRidMap,
            m_ridToVidMap,
            m_linecardKeys);
    sr->hardReinit();

    // perform reinit on the flexcounter groups
    auto flexCounterGroupKeys = m_client->getFlexCounterGroupKeys();    
    auto fgr = std::make_shared<FlexCounterGroupReiniter>(
                m_client,
                m_manager,
                flexCounterGroupKeys);
    fgr->hardReinit();

    // perform reinit on the flexcounters
    auto flexCounterKeys = m_client->getFlexCounterKeys();
    auto fr = std::make_shared<FlexCounterReiniter>(
            m_client,
            m_translator,
            m_manager,
            m_vidToRidMap,
            m_ridToVidMap,
            flexCounterKeys);
    fr->hardReinit();

    return sr->getLinecard();
}
