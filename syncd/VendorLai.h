#pragma once

extern "C" {
#include "lai.h"
}

#include "lib/inc/LaiInterface.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace syncd
{
    class VendorLai :
        public lairedis::LaiInterface
    {
    public:

        VendorLai();

        virtual ~VendorLai();

    public:

        lai_status_t initialize(
            _In_ uint64_t flags,
            _In_ const lai_service_method_table_t* service_method_table) override;

        lai_status_t uninitialize(void) override;

        lai_status_t linkCheck(_Out_ bool *up);

    public: // LAI interface overrides

        virtual lai_status_t create(
            _In_ lai_object_type_t objectType,
            _Out_ lai_object_id_t* objectId,
            _In_ lai_object_id_t linecardId,
            _In_ uint32_t attr_count,
            _In_ const lai_attribute_t* attr_list) override;

        virtual lai_status_t remove(
            _In_ lai_object_type_t objectType,
            _In_ lai_object_id_t objectId) override;

        virtual lai_status_t set(
            _In_ lai_object_type_t objectType,
            _In_ lai_object_id_t objectId,
            _In_ const lai_attribute_t* attr) override;

        virtual lai_status_t get(
            _In_ lai_object_type_t objectType,
            _In_ lai_object_id_t objectId,
            _In_ uint32_t attr_count,
            _Inout_ lai_attribute_t* attr_list) override;

    public: // stats API

        virtual lai_status_t getStats(
            _In_ lai_object_type_t object_type,
            _In_ lai_object_id_t object_id,
            _In_ uint32_t number_of_counters,
            _In_ const lai_stat_id_t* counter_ids,
            _Out_ lai_stat_value_t* counters) override;

        virtual lai_status_t getStatsExt(
            _In_ lai_object_type_t object_type,
            _In_ lai_object_id_t object_id,
            _In_ uint32_t number_of_counters,
            _In_ const lai_stat_id_t* counter_ids,
            _In_ lai_stats_mode_t mode,
            _Out_ lai_stat_value_t* counters) override;

        virtual lai_status_t clearStats(
            _In_ lai_object_type_t object_type,
            _In_ lai_object_id_t object_id,
            _In_ uint32_t number_of_counters,
            _In_ const lai_stat_id_t* counter_ids) override;

    public: // alarms API

        virtual lai_status_t getAlarms(
            _In_ lai_object_type_t object_type,
            _In_ lai_object_id_t object_id,
            _In_ uint32_t number_of_alarms,
            _In_ const lai_alarm_type_t* alarm_ids,
            _Out_ lai_alarm_info_t* alarm_info) override;

        virtual lai_status_t clearAlarms(
            _In_ lai_object_type_t object_type,
            _In_ lai_object_id_t object_id,
            _In_ uint32_t number_of_alarms,
            _In_ const lai_alarm_type_t* alarm_ids) override;

    public: // LAI API

        virtual lai_status_t objectTypeGetAvailability(
            _In_ lai_object_id_t linecardId,
            _In_ lai_object_type_t objectType,
            _In_ uint32_t attrCount,
            _In_ const lai_attribute_t* attrList,
            _Out_ uint64_t* count) override;

        virtual lai_status_t queryAttributeCapability(
            _In_ lai_object_id_t linecard_id,
            _In_ lai_object_type_t object_type,
            _In_ lai_attr_id_t attr_id,
            _Out_ lai_attr_capability_t* capability) override;

        virtual lai_status_t queryAattributeEnumValuesCapability(
            _In_ lai_object_id_t linecard_id,
            _In_ lai_object_type_t object_type,
            _In_ lai_attr_id_t attr_id,
            _Inout_ lai_s32_list_t* enum_values_capability) override;

        virtual lai_object_type_t objectTypeQuery(
            _In_ lai_object_id_t objectId) override;

        virtual lai_object_id_t linecardIdQuery(
            _In_ lai_object_id_t objectId) override;

        virtual lai_status_t logSet(
            _In_ lai_api_t api,
            _In_ lai_log_level_t log_level) override;

    private:

        bool m_apiInitialized;

        std::mutex m_apimutex;

        lai_service_method_table_t m_service_method_table;

        lai_apis_t m_apis;
    };
}
