#pragma once

extern "C" {
#include "lai.h"
}

#include "Lai.h"

#include <memory>

#define PRIVATE __attribute__((visibility("hidden")))

PRIVATE extern const lai_port_api_t            redis_port_api;
PRIVATE extern const lai_linecard_api_t        redis_linecard_api;
PRIVATE extern const lai_transceiver_api_t     redis_transceiver_api;
PRIVATE extern const lai_logicalchannel_api_t  redis_logicalchannel_api;
PRIVATE extern const lai_otn_api_t             redis_otn_api;
PRIVATE extern const lai_ethernet_api_t        redis_ethernet_api;
PRIVATE extern const lai_physicalchannel_api_t redis_physicalchannel_api;
PRIVATE extern const lai_och_api_t             redis_och_api;
PRIVATE extern const lai_lldp_api_t            redis_lldp_api;
PRIVATE extern const lai_assignment_api_t      redis_assignment_api;
PRIVATE extern const lai_interface_api_t       redis_interface_api;
PRIVATE extern const lai_oa_api_t              redis_oa_api;
PRIVATE extern const lai_osc_api_t             redis_osc_api;
PRIVATE extern const lai_aps_api_t             redis_aps_api;
PRIVATE extern const lai_apsport_api_t         redis_apsport_api;
PRIVATE extern const lai_attenuator_api_t      redis_attenuator_api;
PRIVATE extern const lai_wss_api_t             redis_wss_api;
PRIVATE extern const lai_mediachannel_api_t    redis_mediachannel_api;
PRIVATE extern const lai_ocm_api_t             redis_ocm_api;
PRIVATE extern const lai_otdr_api_t            redis_otdr_api;

PRIVATE extern std::shared_ptr<lairedis::Lai>   redis_lai;

// QUAD OID

#define REDIS_CREATE(OT,ot)                             \
    static lai_status_t redis_create_ ## ot(            \
            _Out_ lai_object_id_t *object_id,           \
            _In_ lai_object_id_t linecard_id,             \
            _In_ uint32_t attr_count,                   \
            _In_ const lai_attribute_t *attr_list)      \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return redis_lai->create(                           \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            linecard_id,                                  \
            attr_count,                                 \
            attr_list);                                 \
}

#define REDIS_REMOVE(OT,ot)                             \
    static lai_status_t redis_remove_ ## ot(            \
            _In_ lai_object_id_t object_id)             \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return redis_lai->remove(                           \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id);                                 \
}

#define REDIS_SET(OT,ot)                                \
    static lai_status_t redis_set_ ## ot ## _attribute( \
            _In_ lai_object_id_t object_id,             \
            _In_ const lai_attribute_t *attr)           \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return redis_lai->set(                              \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            attr);                                      \
}

#define REDIS_GET(OT,ot)                                \
    static lai_status_t redis_get_ ## ot ## _attribute( \
            _In_ lai_object_id_t object_id,             \
            _In_ uint32_t attr_count,                   \
            _Inout_ lai_attribute_t *attr_list)         \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return redis_lai->get(                              \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            attr_count,                                 \
            attr_list);                                 \
}

// QUAD DECLARE

#define REDIS_GENERIC_QUAD(OT,ot)  \
    REDIS_CREATE(OT,ot);           \
    REDIS_REMOVE(OT,ot);           \
    REDIS_SET(OT,ot);              \
    REDIS_GET(OT,ot);

// QUAD API

#define REDIS_GENERIC_QUAD_API(ot)      \
    redis_create_ ## ot,                \
    redis_remove_ ## ot,                \
    redis_set_ ## ot ##_attribute,      \
    redis_get_ ## ot ##_attribute,

// STATS

#define REDIS_GET_STATS(OT,ot)                          \
    static lai_status_t redis_get_ ## ot ## _stats(     \
            _In_ lai_object_id_t object_id,             \
            _In_ uint32_t number_of_counters,           \
            _In_ const lai_stat_id_t *counter_ids,      \
            _Out_ lai_stat_value_t *counters)           \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return redis_lai->getStats(                         \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            number_of_counters,                         \
            counter_ids,                                \
            counters);                                  \
}

#define REDIS_GET_STATS_EXT(OT,ot)                      \
    static lai_status_t redis_get_ ## ot ## _stats_ext( \
            _In_ lai_object_id_t object_id,             \
            _In_ uint32_t number_of_counters,           \
            _In_ const lai_stat_id_t *counter_ids,      \
            _In_ lai_stats_mode_t mode,                 \
            _Out_ lai_stat_value_t *counters)           \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return redis_lai->getStatsExt(                      \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            number_of_counters,                         \
            counter_ids,                                \
            mode,                                       \
            counters);                                  \
}

#define REDIS_CLEAR_STATS(OT,ot)                        \
    static lai_status_t redis_clear_ ## ot ## _stats(   \
            _In_ lai_object_id_t object_id,             \
            _In_ uint32_t number_of_counters,           \
            _In_ const lai_stat_id_t *counter_ids)      \
{                                                       \
    SWSS_LOG_ENTER();                                   \
    return redis_lai->clearStats(                       \
            (lai_object_type_t)LAI_OBJECT_TYPE_ ## OT,  \
            object_id,                                  \
            number_of_counters,                         \
            counter_ids);                               \
}

// STATS DECLARE

#define REDIS_GENERIC_STATS(OT, ot)    \
    REDIS_GET_STATS(OT,ot);            \
    REDIS_GET_STATS_EXT(OT,ot);        \
    REDIS_CLEAR_STATS(OT,ot);

// STATS API

#define REDIS_GENERIC_STATS_API(ot)     \
    redis_get_ ## ot ## _stats,         \
    redis_get_ ## ot ## _stats_ext,     \
    redis_clear_ ## ot ## _stats,

