#include "LinecardState.h"
#include "RealObjectIdManager.h"
#include "LinecardStateBase.h"
#include "RealObjectIdManager.h"

#include "swss/logger.h"

#include "meta/otai_serialize.h"

#include <linux/if.h>

using namespace otaivs;

#define VS_COUNTERS_COUNT_MSB (0x80000000)

LinecardState::LinecardState(
        _In_ otai_object_id_t linecard_id,
        _In_ std::shared_ptr<LinecardConfig> config):
    m_linecard_id(linecard_id),
    m_linecardConfig(config)
{
    SWSS_LOG_ENTER();

    if (RealObjectIdManager::objectTypeQuery(linecard_id) != OTAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_THROW("object %s is not LINECARD, its %s",
                otai_serialize_object_id(linecard_id).c_str(),
                otai_serialize_object_type(RealObjectIdManager::objectTypeQuery(linecard_id)).c_str());
    }

    for (int i = OTAI_OBJECT_TYPE_NULL; i < (int)OTAI_OBJECT_TYPE_EXTENSIONS_MAX; ++i)
    {
        /*
         * Populate empty maps for each object to avoid checking if
         * objecttype exists.
         */

        m_objectHash[(otai_object_type_t)i] = { };
    }

    /*
     * Create linecard by default, it will require special treat on
     * creating.
     */

    m_objectHash[OTAI_OBJECT_TYPE_LINECARD][otai_serialize_object_id(linecard_id)] = {};
    otai_attribute_t attr;
    
    attr.id = OTAI_LINECARD_ATTR_SOFTWARE_VERSION;
    strncpy(attr.value.chardata, "1.1.1", sizeof(attr.value.chardata) - 1);
    auto a = std::make_shared<OtaiAttrWrap>(OTAI_OBJECT_TYPE_LINECARD, &attr);
    m_objectHash[OTAI_OBJECT_TYPE_LINECARD][otai_serialize_object_id(linecard_id)][a->getAttrMetadata()->attridname] = a;

    attr.id = OTAI_LINECARD_ATTR_OPER_STATUS;
    attr.value.s32 = OTAI_OPER_STATUS_ACTIVE;
    auto b = std::make_shared<OtaiAttrWrap>(OTAI_OBJECT_TYPE_LINECARD, &attr);
    m_objectHash[OTAI_OBJECT_TYPE_LINECARD][otai_serialize_object_id(linecard_id)][b->getAttrMetadata()->attridname] = b;
    attr.id = OTAI_LINECARD_ATTR_UPGRADE_STATE;
    attr.value.s32 = OTAI_LINECARD_UPGRADE_STATE_IDLE;
    auto c = std::make_shared<OtaiAttrWrap>(OTAI_OBJECT_TYPE_LINECARD, &attr);
    m_objectHash[OTAI_OBJECT_TYPE_LINECARD][otai_serialize_object_id(linecard_id)][c->getAttrMetadata()->attridname] = c;
}

LinecardState::~LinecardState()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("begin");

    SWSS_LOG_NOTICE("linecard %s",
            otai_serialize_object_id(m_linecard_id).c_str());

    SWSS_LOG_NOTICE("end");
}

void LinecardState::setMeta(
        std::weak_ptr<otaimeta::Meta> meta)
{
    SWSS_LOG_ENTER();

    m_meta = meta;
}

otai_object_id_t LinecardState::getLinecardId() const
{
    SWSS_LOG_ENTER();

    return m_linecard_id;
}

otai_status_t LinecardState::getStatsExt(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t* counter_ids,
        _In_ otai_stats_mode_t mode,
        _Out_ otai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    auto str_object_id = otai_serialize_object_id(object_id);

    auto mapit = m_countersMap.find(str_object_id);

    if (mapit == m_countersMap.end())
        m_countersMap[str_object_id] = { };

    auto& localcounters = m_countersMap[str_object_id];

    for (uint32_t i = 0; i < number_of_counters; ++i)
    {
        int32_t id = counter_ids[i];

        auto stat_metadata  = otai_metadata_get_stat_metadata(object_type, id);

        if (stat_metadata->statvaluetype == OTAI_STAT_VALUE_TYPE_UINT64) {
            auto it = localcounters.find(id);

            if (it == localcounters.end())
            {
                // if counter is not found on list, just return 0
                counters[i].u64 = 1;
                localcounters[id] = 1;
            }
            else
            {
                counters[i].u64 = it->second;
            }

            if (mode == OTAI_STATS_MODE_READ_AND_CLEAR)
            {
                localcounters[ id ] = 0;
            }
        } else if (stat_metadata->statvaluetype == OTAI_STAT_VALUE_TYPE_DOUBLE) {
            auto it = localcounters.find(id);
            if (it == localcounters.end())
            {
                localcounters[id] = 1;
                counters[i].d64 = 1.0;
            }
            else
            {
                counters[i].d64 = (double)(it->second);
            }
        }
    }

    return OTAI_STATUS_SUCCESS;
}

std::shared_ptr<otaimeta::Meta> LinecardState::getMeta()
{
    SWSS_LOG_ENTER();

    auto meta = m_meta.lock();

    if (!meta)
    {
        SWSS_LOG_WARN("meta pointer expired");
    }

    return meta;
}
