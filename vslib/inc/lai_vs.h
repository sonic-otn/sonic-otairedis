#pragma once

extern "C" {
#include "lai.h"
}

#include "Lai.h"

#include <memory>

#define PRIVATE __attribute__((visibility("hidden")))

PRIVATE extern const lai_linecard_api_t        vs_linecard_api;
PRIVATE extern const lai_port_api_t            vs_port_api;
PRIVATE extern const lai_transceiver_api_t     vs_transceiver_api;
PRIVATE extern const lai_logicalchannel_api_t  vs_logicalchannel_api;
PRIVATE extern const lai_otn_api_t             vs_otn_api;
PRIVATE extern const lai_ethernet_api_t        vs_ethernet_api;
PRIVATE extern const lai_physicalchannel_api_t vs_physicalchannel_api;
PRIVATE extern const lai_och_api_t             vs_och_api;
PRIVATE extern const lai_lldp_api_t            vs_lldp_api;
PRIVATE extern const lai_assignment_api_t      vs_assignment_api;
PRIVATE extern const lai_interface_api_t       vs_interface_api;
PRIVATE extern const lai_oa_api_t              vs_oa_api;
PRIVATE extern const lai_osc_api_t             vs_osc_api;
PRIVATE extern const lai_aps_api_t             vs_aps_api;
PRIVATE extern const lai_apsport_api_t         vs_apsport_api;
PRIVATE extern const lai_attenuator_api_t      vs_attenuator_api;
PRIVATE extern const lai_wss_api_t             vs_wss_api;
PRIVATE extern const lai_mediachannel_api_t    vs_mediachannel_api;
PRIVATE extern const lai_ocm_api_t             vs_ocm_api;
PRIVATE extern const lai_otdr_api_t            vs_otdr_api;

PRIVATE extern std::shared_ptr<laivs::Lai>      vs_lai;

// QUAD OID

#define VS_CREATE(OT,ot)                                \
    static lai_status_t vs_create_ ## ot(               \
            _Out_ lai_object_id_t *object_id,           \
            _In_ lai_object_id_t linecard_id,             \
            _In_ uint32_t attr_count,                   \
            _In_ const lai_attribute_t *attr_list)      \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_lai->create(                              \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            linecard_id,                                  \
            attr_count,                                 \
            attr_list);                                 \
}

#define VS_REMOVE(OT,ot)                                \
    static lai_status_t vs_remove_ ## ot(               \
            _In_ lai_object_id_t object_id)             \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_lai->remove(                              \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id);                                 \
}

#define VS_SET(OT,ot)                                   \
    static lai_status_t vs_set_ ## ot ## _attribute(    \
            _In_ lai_object_id_t object_id,             \
            _In_ const lai_attribute_t *attr)           \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_lai->set(                                 \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            attr);                                      \
}

#define VS_GET(OT,ot)                                   \
    static lai_status_t vs_get_ ## ot ## _attribute(    \
            _In_ lai_object_id_t object_id,             \
            _In_ uint32_t attr_count,                   \
            _Inout_ lai_attribute_t *attr_list)         \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_lai->get(                                 \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
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
    static lai_status_t vs_get_ ## ot ## _stats(        \
            _In_ lai_object_id_t object_id,             \
            _In_ uint32_t number_of_counters,           \
            _In_ const lai_stat_id_t *counter_ids,      \
            _Out_ lai_stat_value_t *counters)           \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_lai->getStats(                            \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            number_of_counters,                         \
            counter_ids,                                \
            counters);                                  \
}

#define VS_GET_STATS_EXT(OT,ot)                         \
    static lai_status_t vs_get_ ## ot ## _stats_ext(    \
            _In_ lai_object_id_t object_id,             \
            _In_ uint32_t number_of_counters,           \
            _In_ const lai_stat_id_t *counter_ids,      \
            _In_ lai_stats_mode_t mode,                 \
            _Out_ lai_stat_value_t *counters)           \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_lai->getStatsExt(                         \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            number_of_counters,                         \
            counter_ids,                                \
            mode,                                       \
            counters);                                  \
}

#define VS_CLEAR_STATS(OT,ot)                           \
    static lai_status_t vs_clear_ ## ot ## _stats(      \
            _In_ lai_object_id_t object_id,             \
            _In_ uint32_t number_of_counters,           \
            _In_ const lai_stat_id_t *counter_ids)      \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_lai->clearStats(                          \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
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



// ALARMS

#define VS_GET_ALARMS(OT,ot)                             \
    static lai_status_t vs_get_ ## ot ## _alarms(        \
            _In_ lai_object_id_t object_id,             \
            _In_ uint32_t number_of_alarms,           \
            _In_ const lai_alarm_type_t *alarm_ids,      \
            _Out_ lai_alarm_info_t *alarm_info)                   \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_lai->getAlarms(                            \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            number_of_alarms,                         \
            alarm_ids,                                \
            alarm_info);                                  \
}

#define VS_CLEAR_ALARMS(OT,ot)                           \
    static lai_status_t vs_clear_ ## ot ## _alarms(      \
            _In_ lai_object_id_t object_id,             \
            _In_ uint32_t number_of_alarms,           \
            _In_ const lai_alarm_type_t *alarm_ids)      \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return vs_lai->clearAlarms(                          \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            number_of_alarms,                         \
            alarm_ids);                               \
}

// ALARMS DECLARE

#define VS_GENERIC_ALARMS(OT, ot)    \
    VS_GET_ALARMS(OT,ot);            \
    VS_CLEAR_ALARMS(OT,ot);

// ALARMS API

#define VS_GENERIC_ALARMS_API(ot)    \
    vs_get_ ## ot ## _alarms,        \
    vs_clear_ ## ot ## _alarms,

