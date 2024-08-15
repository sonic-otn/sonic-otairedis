#pragma once

extern "C" {
#include "otai.h"
}

#include "Otai.h"

#include <memory>

#define PRIVATE __attribute__((visibility("hidden")))

PRIVATE extern const otai_linecard_api_t        vs_linecard_api;
PRIVATE extern const otai_port_api_t            vs_port_api;
PRIVATE extern const otai_transceiver_api_t     vs_transceiver_api;
PRIVATE extern const otai_logicalchannel_api_t  vs_logicalchannel_api;
PRIVATE extern const otai_otn_api_t             vs_otn_api;
PRIVATE extern const otai_ethernet_api_t        vs_ethernet_api;
PRIVATE extern const otai_physicalchannel_api_t vs_physicalchannel_api;
PRIVATE extern const otai_och_api_t             vs_och_api;
PRIVATE extern const otai_lldp_api_t            vs_lldp_api;
PRIVATE extern const otai_assignment_api_t      vs_assignment_api;
PRIVATE extern const otai_interface_api_t       vs_interface_api;
PRIVATE extern const otai_oa_api_t              vs_oa_api;
PRIVATE extern const otai_osc_api_t             vs_osc_api;
PRIVATE extern const otai_aps_api_t             vs_aps_api;
PRIVATE extern const otai_apsport_api_t         vs_apsport_api;
PRIVATE extern const otai_attenuator_api_t      vs_attenuator_api;
PRIVATE extern const otai_wss_api_t             vs_wss_api;
PRIVATE extern const otai_mediachannel_api_t    vs_mediachannel_api;
PRIVATE extern const otai_ocm_api_t             vs_ocm_api;
PRIVATE extern const otai_otdr_api_t            vs_otdr_api;

PRIVATE extern std::shared_ptr<otaivs::Otai>      vs_otai;

// QUAD OID

#define VS_CREATE(OT,ot)                                \
    static otai_status_t vs_create_ ## ot(               \
            _Out_ otai_object_id_t *object_id,           \
            _In_ otai_object_id_t linecard_id,             \
            _In_ uint32_t attr_count,                   \
            _In_ const otai_attribute_t *attr_list)      \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_otai->create(                              \
            (otai_object_type_t)OTAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            linecard_id,                                  \
            attr_count,                                 \
            attr_list);                                 \
}

#define VS_REMOVE(OT,ot)                                \
    static otai_status_t vs_remove_ ## ot(               \
            _In_ otai_object_id_t object_id)             \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_otai->remove(                              \
            (otai_object_type_t)OTAI_OBJECT_TYPE_ ## OT,  \
            object_id);                                 \
}

#define VS_SET(OT,ot)                                   \
    static otai_status_t vs_set_ ## ot ## _attribute(    \
            _In_ otai_object_id_t object_id,             \
            _In_ const otai_attribute_t *attr)           \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_otai->set(                                 \
            (otai_object_type_t)OTAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            attr);                                      \
}

#define VS_GET(OT,ot)                                   \
    static otai_status_t vs_get_ ## ot ## _attribute(    \
            _In_ otai_object_id_t object_id,             \
            _In_ uint32_t attr_count,                   \
            _Inout_ otai_attribute_t *attr_list)         \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_otai->get(                                 \
            (otai_object_type_t)OTAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            attr_count,                                 \
            attr_list);                                 \
}

// QUAD DECLARE

#define VS_GENERIC_QUAD(OT,ot)  \
    VS_CREATE(OT,ot);           \
    VS_REMOVE(OT,ot);           \
    VS_SET(OT,ot);              \
    VS_GET(OT,ot);

#define VS_GENERIC_QUAD_API(ot)     \
    vs_create_ ## ot,               \
    vs_remove_ ## ot,               \
    vs_set_ ## ot ##_attribute,     \
    vs_get_ ## ot ##_attribute,

// STATS

#define VS_GET_STATS(OT,ot)                             \
    static otai_status_t vs_get_ ## ot ## _stats(        \
            _In_ otai_object_id_t object_id,             \
            _In_ uint32_t number_of_counters,           \
            _In_ const otai_stat_id_t *counter_ids,      \
            _Out_ otai_stat_value_t *counters)           \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_otai->getStats(                            \
            (otai_object_type_t)OTAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            number_of_counters,                         \
            counter_ids,                                \
            counters);                                  \
}

#define VS_GET_STATS_EXT(OT,ot)                         \
    static otai_status_t vs_get_ ## ot ## _stats_ext(    \
            _In_ otai_object_id_t object_id,             \
            _In_ uint32_t number_of_counters,           \
            _In_ const otai_stat_id_t *counter_ids,      \
            _In_ otai_stats_mode_t mode,                 \
            _Out_ otai_stat_value_t *counters)           \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_otai->getStatsExt(                         \
            (otai_object_type_t)OTAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            number_of_counters,                         \
            counter_ids,                                \
            mode,                                       \
            counters);                                  \
}

#define VS_CLEAR_STATS(OT,ot)                           \
    static otai_status_t vs_clear_ ## ot ## _stats(      \
            _In_ otai_object_id_t object_id,             \
            _In_ uint32_t number_of_counters,           \
            _In_ const otai_stat_id_t *counter_ids)      \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_otai->clearStats(                          \
            (otai_object_type_t)OTAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            number_of_counters,                         \
            counter_ids);                               \
}

// STATS DECLARE

#define VS_GENERIC_STATS(OT, ot)    \
    VS_GET_STATS(OT,ot);            \
    VS_GET_STATS_EXT(OT,ot);        \
    VS_CLEAR_STATS(OT,ot);

// STATS API

#define VS_GENERIC_STATS_API(ot)    \
    vs_get_ ## ot ## _stats,        \
    vs_get_ ## ot ## _stats_ext,    \
    vs_clear_ ## ot ## _stats,

