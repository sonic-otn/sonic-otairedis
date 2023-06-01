#pragma once

#include "RedisRemoteLaiInterface.h"
#include "Notification.h"
#include "Recorder.h"
#include "Context.h"

#include "meta/Meta.h"

#include "swss/logger.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <map>

namespace lairedis
{
    class Lai:
        public lairedis::LaiInterface
    {
        public:

            Lai();

            virtual ~Lai();

        public:

            lai_status_t initialize(
                    _In_ uint64_t flags,
                    _In_ const lai_service_method_table_t *service_method_table);

            lai_status_t uninitialize(void) override;

            virtual lai_status_t linkCheck(_Out_ bool *up) override;

        public: // LAI interface overrides

            virtual lai_status_t create(
                    _In_ lai_object_type_t objectType,
                    _Out_ lai_object_id_t* objectId,
                    _In_ lai_object_id_t linecardId,
                    _In_ uint32_t attr_count,
                    _In_ const lai_attribute_t *attr_list) override;

            virtual lai_status_t remove(
                    _In_ lai_object_type_t objectType,
                    _In_ lai_object_id_t objectId) override;

            virtual lai_status_t set(
                    _In_ lai_object_type_t objectType,
                    _In_ lai_object_id_t objectId,
                    _In_ const lai_attribute_t *attr) override;

            virtual lai_status_t get(
                    _In_ lai_object_type_t objectType,
                    _In_ lai_object_id_t objectId,
                    _In_ uint32_t attr_count,
                    _Inout_ lai_attribute_t *attr_list) override;

        public: // stats API

            virtual lai_status_t getStats(
                    _In_ lai_object_type_t object_type,
                    _In_ lai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const lai_stat_id_t *counter_ids,
                    _Out_ lai_stat_value_t *counters) override;

            virtual lai_status_t getStatsExt(
                    _In_ lai_object_type_t object_type,
                    _In_ lai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const lai_stat_id_t *counter_ids,
                    _In_ lai_stats_mode_t mode,
                    _Out_ lai_stat_value_t *counters) override;

            virtual lai_status_t clearStats(
                    _In_ lai_object_type_t object_type,
                    _In_ lai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const lai_stat_id_t *counter_ids) override;

        public: // LAI API

            virtual lai_status_t objectTypeGetAvailability(
                    _In_ lai_object_id_t linecardId,
                    _In_ lai_object_type_t objectType,
                    _In_ uint32_t attrCount,
                    _In_ const lai_attribute_t *attrList,
                    _Out_ uint64_t *count) override;

            virtual lai_status_t queryAttributeCapability(
                    _In_ lai_object_id_t linecard_id,
                    _In_ lai_object_type_t object_type,
                    _In_ lai_attr_id_t attr_id,
                    _Out_ lai_attr_capability_t *capability) override;

            virtual lai_status_t queryAattributeEnumValuesCapability(
                    _In_ lai_object_id_t linecard_id,
                    _In_ lai_object_type_t object_type,
                    _In_ lai_attr_id_t attr_id,
                    _Inout_ lai_s32_list_t *enum_values_capability) override;

            virtual lai_object_type_t objectTypeQuery(
                    _In_ lai_object_id_t objectId) override;

            virtual lai_object_id_t linecardIdQuery(
                    _In_ lai_object_id_t objectId) override;

            virtual lai_status_t logSet(
                    _In_ lai_api_t api,
                    _In_ lai_log_level_t log_level) override;

        private:

            lai_linecard_notifications_t handle_notification(
                    _In_ std::shared_ptr<Notification> notification,
                    _In_ Context* context);

            std::shared_ptr<Context> getContext(
                    _In_ uint32_t globalContext);

        private:

            bool m_apiInitialized;

            std::recursive_mutex m_apimutex;

            std::map<uint32_t, std::shared_ptr<Context>> m_contextMap;

            lai_service_method_table_t m_service_method_table;

            std::shared_ptr<Recorder> m_recorder;
    };
}
