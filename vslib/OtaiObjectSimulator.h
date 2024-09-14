#pragma once

#include "swss/logger.h"
#include "otaitypes.h"
#include <nlohmann/json.hpp>

namespace otaivs
{
    class OtaiObjectSimulator 
    {
        public:
            static std::shared_ptr<OtaiObjectSimulator> getOtaiObjectSimulator(
                _In_ otai_object_type_t object_type);
        private:
            static bool isObjectSimulatorInitialed;

        public:
            OtaiObjectSimulator(
                _In_ otai_object_type_t object_type);
            
            virtual otai_status_t create(
                _Out_ otai_object_id_t* objectId,
                _In_ otai_object_id_t linecardId,
                _In_ uint32_t attr_count,
                _In_ const otai_attribute_t *attr_list);

            virtual otai_status_t remove(
                _In_ otai_object_id_t objectId);

            virtual otai_status_t set(
                _In_ otai_object_id_t objectId,
                _In_ const otai_attribute_t *attr); 

            virtual otai_status_t get(
                _In_ otai_object_id_t objectId,
                _In_ uint32_t attr_count,
                _Inout_ otai_attribute_t *attr_list);

            virtual otai_status_t getStatsExt(
                _In_ otai_object_id_t object_id,
                _In_ uint32_t number_of_counters,
                _In_ const otai_stat_id_t* counter_ids,
                _In_ otai_stats_mode_t mode,
                _Out_ otai_stat_value_t *counters);

            virtual otai_status_t clearStats(
                _In_ otai_object_id_t object_id,
                _In_ uint32_t number_of_counters,
                _In_ const otai_stat_id_t *counter_ids);

        private:
            nlohmann::json m_data;
            otai_object_type_t m_objectType;
    };
}
