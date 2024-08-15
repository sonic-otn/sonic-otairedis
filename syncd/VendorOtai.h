#pragma once

extern "C" {
#include "otai.h"
}

#include "meta/OtaiInterface.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace syncd
{
    class VendorOtai :
        public otairedis::OtaiInterface
    {
    public:

        VendorOtai();

        virtual ~VendorOtai();

    public:

        otai_status_t initialize(
            _In_ uint64_t flags,
            _In_ const otai_service_method_table_t* service_method_table) override;

        otai_status_t uninitialize(void) override;

        otai_status_t linkCheck(_Out_ bool *up);

    public: // OTAI interface overrides

        virtual otai_status_t create(
            _In_ otai_object_type_t objectType,
            _Out_ otai_object_id_t* objectId,
            _In_ otai_object_id_t linecardId,
            _In_ uint32_t attr_count,
            _In_ const otai_attribute_t* attr_list) override;

        virtual otai_status_t remove(
            _In_ otai_object_type_t objectType,
            _In_ otai_object_id_t objectId) override;

        virtual otai_status_t set(
            _In_ otai_object_type_t objectType,
            _In_ otai_object_id_t objectId,
            _In_ const otai_attribute_t* attr) override;

        virtual otai_status_t get(
            _In_ otai_object_type_t objectType,
            _In_ otai_object_id_t objectId,
            _In_ uint32_t attr_count,
            _Inout_ otai_attribute_t* attr_list) override;

    public: // stats API

        virtual otai_status_t getStats(
            _In_ otai_object_type_t object_type,
            _In_ otai_object_id_t object_id,
            _In_ uint32_t number_of_counters,
            _In_ const otai_stat_id_t* counter_ids,
            _Out_ otai_stat_value_t* counters) override;

        virtual otai_status_t getStatsExt(
            _In_ otai_object_type_t object_type,
            _In_ otai_object_id_t object_id,
            _In_ uint32_t number_of_counters,
            _In_ const otai_stat_id_t* counter_ids,
            _In_ otai_stats_mode_t mode,
            _Out_ otai_stat_value_t* counters) override;

        virtual otai_status_t clearStats(
            _In_ otai_object_type_t object_type,
            _In_ otai_object_id_t object_id,
            _In_ uint32_t number_of_counters,
            _In_ const otai_stat_id_t* counter_ids) override;

    public: // OTAI API
        virtual otai_object_type_t objectTypeQuery(
            _In_ otai_object_id_t objectId) override;

        virtual otai_object_id_t linecardIdQuery(
            _In_ otai_object_id_t objectId) override;

        virtual otai_status_t logSet(
            _In_ otai_api_t api,
            _In_ otai_log_level_t log_level) override;

    private:

        bool m_apiInitialized;

        std::mutex m_apimutex;

        otai_service_method_table_t m_service_method_table;

        otai_apis_t m_apis;
    };
}
