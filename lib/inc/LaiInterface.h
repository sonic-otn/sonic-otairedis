#pragma once

extern "C" {
#include "lai.h"
#include "laimetadata.h"
}

#include "lairedis.h"

namespace lairedis
{
    class LaiInterface
    {
        public:

            LaiInterface() = default;

            virtual ~LaiInterface() = default;

        public:

            virtual lai_status_t initialize(
                    _In_ uint64_t flags,
                    _In_ const lai_service_method_table_t *service_method_table) = 0;

            virtual lai_status_t uninitialize(void) = 0;

            virtual lai_status_t linkCheck(_Out_ bool *up) = 0;

        public: // QUAD oid

            virtual lai_status_t create(
                    _In_ lai_object_type_t objectType,
                    _Out_ lai_object_id_t* objectId,
                    _In_ lai_object_id_t linecardId,
                    _In_ uint32_t attr_count,
                    _In_ const lai_attribute_t *attr_list) = 0;

            virtual lai_status_t remove(
                    _In_ lai_object_type_t objectType,
                    _In_ lai_object_id_t objectId) = 0;

            virtual lai_status_t set(
                    _In_ lai_object_type_t objectType,
                    _In_ lai_object_id_t objectId,
                    _In_ const lai_attribute_t *attr) = 0;

            virtual lai_status_t get(
                    _In_ lai_object_type_t objectType,
                    _In_ lai_object_id_t objectId,
                    _In_ uint32_t attr_count,
                    _Inout_ lai_attribute_t *attr_list) = 0;

        public: // QUAD meta key

            virtual lai_status_t create(
                    _Inout_ lai_object_meta_key_t& metaKey,
                    _In_ lai_object_id_t linecard_id,
                    _In_ uint32_t attr_count,
                    _In_ const lai_attribute_t *attr_list);

            virtual lai_status_t remove(
                    _In_ const lai_object_meta_key_t& metaKey);

            virtual lai_status_t set(
                    _In_ const lai_object_meta_key_t& metaKey,
                    _In_ const lai_attribute_t *attr);

            virtual lai_status_t get(
                    _In_ const lai_object_meta_key_t& metaKey,
                    _In_ uint32_t attr_count,
                    _Inout_ lai_attribute_t *attr_list);

        public: // stats API

            virtual lai_status_t getStats(
                    _In_ lai_object_type_t object_type,
                    _In_ lai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const lai_stat_id_t *counter_ids,
                    _Out_ lai_stat_value_t *counters) = 0;

            virtual lai_status_t getStatsExt(
                    _In_ lai_object_type_t object_type,
                    _In_ lai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const lai_stat_id_t *counter_ids,
                    _In_ lai_stats_mode_t mode,
                    _Out_ lai_stat_value_t *counters) = 0;

            virtual lai_status_t clearStats(
                    _In_ lai_object_type_t object_type,
                    _In_ lai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const lai_stat_id_t *counter_ids) = 0;

        public: // LAI API

            virtual lai_status_t objectTypeGetAvailability(
                    _In_ lai_object_id_t linecardId,
                    _In_ lai_object_type_t objectType,
                    _In_ uint32_t attrCount,
                    _In_ const lai_attribute_t *attrList,
                    _Out_ uint64_t *count) = 0;

            virtual lai_status_t queryAttributeCapability(
                    _In_ lai_object_id_t linecard_id,
                    _In_ lai_object_type_t object_type,
                    _In_ lai_attr_id_t attr_id,
                    _Out_ lai_attr_capability_t *capability) = 0;

            virtual lai_status_t queryAattributeEnumValuesCapability(
                    _In_ lai_object_id_t linecard_id,
                    _In_ lai_object_type_t object_type,
                    _In_ lai_attr_id_t attr_id,
                    _Inout_ lai_s32_list_t *enum_values_capability) = 0;

            virtual lai_object_type_t objectTypeQuery(
                    _In_ lai_object_id_t objectId) = 0;

            virtual lai_object_id_t linecardIdQuery(
                    _In_ lai_object_id_t objectId) = 0;

            virtual lai_status_t logSet(
                    _In_ lai_api_t api,
                    _In_ lai_log_level_t log_level) = 0;

    };
}
