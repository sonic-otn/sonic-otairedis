#pragma once

extern "C" {
#include "otai.h"
#include "otaimetadata.h"
}

#include "otairedis.h"

namespace otairedis
{
    class OtaiInterface
    {
        public:

            OtaiInterface() = default;

            virtual ~OtaiInterface() = default;

        public:

            virtual otai_status_t initialize(
                    _In_ uint64_t flags,
                    _In_ const otai_service_method_table_t *service_method_table) = 0;

            virtual otai_status_t uninitialize(void) = 0;

            virtual otai_status_t linkCheck(_Out_ bool *up) = 0;

        public: // QUAD oid

            virtual otai_status_t create(
                    _In_ otai_object_type_t objectType,
                    _Out_ otai_object_id_t* objectId,
                    _In_ otai_object_id_t linecardId,
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list) = 0;

            virtual otai_status_t remove(
                    _In_ otai_object_type_t objectType,
                    _In_ otai_object_id_t objectId) = 0;

            virtual otai_status_t set(
                    _In_ otai_object_type_t objectType,
                    _In_ otai_object_id_t objectId,
                    _In_ const otai_attribute_t *attr) = 0;

            virtual otai_status_t get(
                    _In_ otai_object_type_t objectType,
                    _In_ otai_object_id_t objectId,
                    _In_ uint32_t attr_count,
                    _Inout_ otai_attribute_t *attr_list) = 0;

        public: // QUAD meta key

            virtual otai_status_t create(
                    _Inout_ otai_object_meta_key_t& metaKey,
                    _In_ otai_object_id_t linecard_id,
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

            virtual otai_status_t remove(
                    _In_ const otai_object_meta_key_t& metaKey);

            virtual otai_status_t set(
                    _In_ const otai_object_meta_key_t& metaKey,
                    _In_ const otai_attribute_t *attr);

            virtual otai_status_t get(
                    _In_ const otai_object_meta_key_t& metaKey,
                    _In_ uint32_t attr_count,
                    _Inout_ otai_attribute_t *attr_list);

        public: // stats API

            virtual otai_status_t getStats(
                    _In_ otai_object_type_t object_type,
                    _In_ otai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const otai_stat_id_t *counter_ids,
                    _Out_ otai_stat_value_t *counters) = 0;

            virtual otai_status_t getStatsExt(
                    _In_ otai_object_type_t object_type,
                    _In_ otai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const otai_stat_id_t *counter_ids,
                    _In_ otai_stats_mode_t mode,
                    _Out_ otai_stat_value_t *counters) = 0;

            virtual otai_status_t clearStats(
                    _In_ otai_object_type_t object_type,
                    _In_ otai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const otai_stat_id_t *counter_ids) = 0;

        public: // OTAI API

            virtual otai_status_t objectTypeGetAvailability(
                    _In_ otai_object_id_t linecardId,
                    _In_ otai_object_type_t objectType,
                    _In_ uint32_t attrCount,
                    _In_ const otai_attribute_t *attrList,
                    _Out_ uint64_t *count) = 0;

            virtual otai_status_t queryAttributeCapability(
                    _In_ otai_object_id_t linecard_id,
                    _In_ otai_object_type_t object_type,
                    _In_ otai_attr_id_t attr_id,
                    _Out_ otai_attr_capability_t *capability) = 0;

            virtual otai_status_t queryAattributeEnumValuesCapability(
                    _In_ otai_object_id_t linecard_id,
                    _In_ otai_object_type_t object_type,
                    _In_ otai_attr_id_t attr_id,
                    _Inout_ otai_s32_list_t *enum_values_capability) = 0;

            virtual otai_object_type_t objectTypeQuery(
                    _In_ otai_object_id_t objectId) = 0;

            virtual otai_object_id_t linecardIdQuery(
                    _In_ otai_object_id_t objectId) = 0;

            virtual otai_status_t logSet(
                    _In_ otai_api_t api,
                    _In_ otai_log_level_t log_level) = 0;

    };
}
