#pragma once

#include "RemoteLaiInterface.h"
#include "LinecardContainer.h"
#include "VirtualObjectIdManager.h"
#include "Notification.h"
#include "Recorder.h"
#include "RedisVidIndexGenerator.h"
#include "SkipRecordAttrContainer.h"
#include "RedisChannel.h"
#include "LinecardConfigContainer.h"
#include "ContextConfig.h"

#include "swss/producertable.h"
#include "swss/consumertable.h"
#include "swss/notificationconsumer.h"
#include "swss/selectableevent.h"

#include <memory>
#include <functional>
#include <map>

namespace lairedis
{
    class RedisRemoteLaiInterface:
        public RemoteLaiInterface
    {
        public:

            RedisRemoteLaiInterface(
                    _In_ std::shared_ptr<ContextConfig> contextConfig,
                    _In_ std::function<lai_linecard_notifications_t(std::shared_ptr<Notification>)> notificationCallback,
                    _In_ std::shared_ptr<Recorder> recorder);

            virtual ~RedisRemoteLaiInterface();

        public:

            virtual lai_status_t initialize(
                    _In_ uint64_t flags,
                    _In_ const lai_service_method_table_t *service_method_table);

            virtual lai_status_t uninitialize(void) override;

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

        public: // alarms API

            virtual lai_status_t getAlarms(
                    _In_ lai_object_type_t object_type,
                    _In_ lai_object_id_t object_id,
                    _In_ uint32_t number_of_alarms,
                    _In_ const lai_alarm_type_t *alarm_ids,
                    _Out_ lai_alarm_info_t *alarm_info) override;

            virtual lai_status_t clearAlarms(
                    _In_ lai_object_type_t object_type,
                    _In_ lai_object_id_t object_id,
                    _In_ uint32_t number_of_alarms,
                    _In_ const lai_alarm_type_t *alarm_ids) override;

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

        public: // notify syncd

            virtual lai_status_t notifySyncd(
                    _In_ lai_object_id_t linecardId,
                    _In_ lai_redis_notify_syncd_t redisNotifySyncd) override;

        public:

            /**
             * @brief Checks whether attribute is custom LAI_REDIS_SWITCH attribute.
             *
             * This function should only be used on linecard_api set function.
             */
            static bool isRedisAttribute(
                    _In_ lai_object_id_t obejctType,
                    _In_ const lai_attribute_t* attr);

            lai_status_t setRedisAttribute(
                    _In_ lai_object_id_t linecardId,
                    _In_ const lai_attribute_t* attr);

            void setMeta(
                    _In_ std::weak_ptr<laimeta::Meta> meta);

            lai_linecard_notifications_t syncProcessNotification(
                    _In_ std::shared_ptr<Notification> notification);

        private: // QUAD API helpers

            lai_status_t create(
                    _In_ lai_object_type_t objectType,
                    _In_ const std::string& serializedObjectId,
                    _In_ uint32_t attr_count,
                    _In_ const lai_attribute_t *attr_list);

            lai_status_t remove(
                    _In_ lai_object_type_t objectType,
                    _In_ const std::string& serializedObjectId);

            lai_status_t set(
                    _In_ lai_object_type_t objectType,
                    _In_ const std::string& serializedObjectId,
                    _In_ const lai_attribute_t *attr);

            lai_status_t get(
                    _In_ lai_object_type_t objectType,
                    _In_ const std::string& serializedObjectId,
                    _In_ uint32_t attr_count,
                    _Inout_ lai_attribute_t *attr_list);

        private: // QUAD API response

            /**
             * @brief Wait for response.
             *
             * Will wait for response from syncd. Method used only for single
             * object create/remove/set since they have common output which is
             * lai_status_t.
             */
            lai_status_t waitForResponse(
                    _In_ lai_common_api_t api);

            /**
             * @brief Wait for GET response.
             *
             * Will wait for response from syncd. Method only used for single
             * GET object. If status is SUCCESS all values will be deserialized
             * and transferred to user buffers. If status is BUFFER_OVERFLOW
             * then all non list values will be transferred, but LIST objects
             * will only transfer COUNT item of list, without touching user
             * list at all.
             */
            lai_status_t waitForGetResponse(
                    _In_ lai_object_type_t objectType,
                    _In_ uint32_t attr_count,
                    _Inout_ lai_attribute_t *attr_list);

        private: // stats API response

            lai_status_t waitForGetStatsResponse(
                    _In_ lai_object_type_t object_type,
                    _In_ uint32_t number_of_counters,
                    _In_ const lai_stat_id_t *counter_ids,
                    _Out_ lai_stat_value_t *counters);

            lai_status_t waitForClearStatsResponse();

        private: // alarms API response

            lai_status_t waitForGetAlarmsResponse(
                    _In_ uint32_t number_of_alarms,
                    _Out_ lai_alarm_info_t *alarm_info);

            lai_status_t waitForClearAlarmsResponse();

        private: // bulk QUAD API response

            /**
             * @brief Wait for bulk response.
             *
             * Will wait for response from syncd. Method used only for bulk
             * object create/remove/set since they have common output which is
             * lai_status_t and object_statuses.
             */
            lai_status_t waitForBulkResponse(
                    _In_ lai_common_api_t api,
                    _In_ uint32_t object_count,
                    _Out_ lai_status_t *object_statuses);

        private: // LAI API response

            lai_status_t waitForQueryAttributeCapabilityResponse(
                    _Out_ lai_attr_capability_t* capability);

            lai_status_t waitForQueryAattributeEnumValuesCapabilityResponse(
                    _Inout_ lai_s32_list_t* enumValuesCapability);

            lai_status_t waitForObjectTypeGetAvailabilityResponse(
                    _In_ uint64_t *count);

        private: // notify syncd response

            lai_status_t waitForNotifySyncdResponse();

        private: // notification

            void notificationThreadFunction();

            void handleNotification(
                    _In_ const std::string &name,
                    _In_ const std::string &serializedNotification,
                    _In_ const std::vector<swss::FieldValueTuple> &values);

            lai_status_t setRedisExtensionAttribute(
                    _In_ lai_object_type_t objectType,
                    _In_ lai_object_id_t objectId,
                    _In_ const lai_attribute_t *attr);

        private:

            lai_status_t lai_redis_notify_syncd(
                    _In_ lai_object_id_t linecardId,
                    _In_ const lai_attribute_t *attr);

            void clear_local_state();

            lai_linecard_notifications_t processNotification(
                    _In_ std::shared_ptr<Notification> notification);

            std::string getHardwareInfo(
                    _In_ uint32_t attrCount,
                    _In_ const lai_attribute_t *attrList) const;

        private:

            std::shared_ptr<ContextConfig> m_contextConfig;

            bool m_asicInitViewMode;

            bool m_useTempView;

            bool m_syncMode;

            lai_redis_communication_mode_t m_redisCommunicationMode;

            bool m_initialized;

            std::shared_ptr<Recorder> m_recorder;

            std::shared_ptr<LinecardContainer> m_linecardContainer;

            std::shared_ptr<VirtualObjectIdManager> m_virtualObjectIdManager;

            std::shared_ptr<swss::DBConnector> m_db;

            std::shared_ptr<RedisVidIndexGenerator> m_redisVidIndexGenerator;

            std::weak_ptr<laimeta::Meta> m_meta;

            std::shared_ptr<SkipRecordAttrContainer> m_skipRecordAttrContainer;

            std::shared_ptr<Channel> m_communicationChannel;

            uint64_t m_responseTimeoutMs;

            std::function<lai_linecard_notifications_t(std::shared_ptr<Notification>)> m_notificationCallback;

    };
}
