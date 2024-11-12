#pragma once

#include "LinecardContainer.h"
#include "VirtualObjectIdManager.h"
#include "RedisVidIndexGenerator.h"
#include "RedisChannel.h"
#include "ContextConfig.h"

#include "meta/Notification.h"
#include "meta/OtaiInterface.h"

#include "swss/producertable.h"
#include "swss/consumertable.h"
#include "swss/notificationconsumer.h"
#include "swss/selectableevent.h"

#include <memory>
#include <functional>
#include <map>

namespace otairedis
{
    class RedisRemoteOtaiInterface:
        public OtaiInterface
    {
        public:

            RedisRemoteOtaiInterface(
                    _In_ std::shared_ptr<ContextConfig> contextConfig,
                    _In_ std::function<otai_linecard_notifications_t(std::shared_ptr<Notification>)> notificationCallback);

            virtual ~RedisRemoteOtaiInterface();

        public:

            virtual otai_status_t initialize(
                    _In_ uint64_t flags,
                    _In_ const otai_service_method_table_t *service_method_table);

            virtual otai_status_t uninitialize(void) override;

            virtual otai_status_t linkCheck(_Out_ bool *up) override;

        public: // OTAI interface overrides

            virtual otai_status_t create(
                    _In_ otai_object_type_t objectType,
                    _Out_ otai_object_id_t* objectId,
                    _In_ otai_object_id_t linecardId,
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list) override;

            virtual otai_status_t remove(
                    _In_ otai_object_type_t objectType,
                    _In_ otai_object_id_t objectId) override;

            virtual otai_status_t set(
                    _In_ otai_object_type_t objectType,
                    _In_ otai_object_id_t objectId,
                    _In_ const otai_attribute_t *attr) override;

            virtual otai_status_t get(
                    _In_ otai_object_type_t objectType,
                    _In_ otai_object_id_t objectId,
                    _In_ uint32_t attr_count,
                    _Inout_ otai_attribute_t *attr_list) override;

        public: // stats API

            virtual otai_status_t getStats(
                    _In_ otai_object_type_t object_type,
                    _In_ otai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const otai_stat_id_t *counter_ids,
                    _Out_ otai_stat_value_t *counters) override;

            virtual otai_status_t getStatsExt(
                    _In_ otai_object_type_t object_type,
                    _In_ otai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const otai_stat_id_t *counter_ids,
                    _In_ otai_stats_mode_t mode,
                    _Out_ otai_stat_value_t *counters) override;

            virtual otai_status_t clearStats(
                    _In_ otai_object_type_t object_type,
                    _In_ otai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const otai_stat_id_t *counter_ids) override;

        public: // OTAI API
            virtual otai_object_type_t objectTypeQuery(
                    _In_ otai_object_id_t objectId) override;

            virtual otai_object_id_t linecardIdQuery(
                    _In_ otai_object_id_t objectId) override;

            virtual otai_status_t logSet(
                    _In_ otai_api_t api,
                    _In_ otai_log_level_t log_level) override;

        public:

            /**
             * @brief Checks whether attribute is custom OTAI_REDIS_SWITCH attribute.
             *
             * This function should only be used on linecard_api set function.
             */
            static bool isRedisAttribute(
                    _In_ otai_object_id_t obejctType,
                    _In_ const otai_attribute_t* attr);

            void setMeta(
                    _In_ std::weak_ptr<otaimeta::Meta> meta);

            otai_linecard_notifications_t syncProcessNotification(
                    _In_ std::shared_ptr<Notification> notification);

        private: // QUAD API helpers

            otai_status_t create(
                    _In_ otai_object_type_t objectType,
                    _In_ const std::string& serializedObjectId,
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

            otai_status_t remove(
                    _In_ otai_object_type_t objectType,
                    _In_ const std::string& serializedObjectId);

            otai_status_t set(
                    _In_ otai_object_type_t objectType,
                    _In_ const std::string& serializedObjectId,
                    _In_ const otai_attribute_t *attr);

            otai_status_t get(
                    _In_ otai_object_type_t objectType,
                    _In_ const std::string& serializedObjectId,
                    _In_ uint32_t attr_count,
                    _Inout_ otai_attribute_t *attr_list);

        private: // QUAD API response

            /**
             * @brief Wait for response.
             *
             * Will wait for response from syncd. Method used only for single
             * object create/remove/set since they have common output which is
             * otai_status_t.
             */
            otai_status_t waitForResponse(
                    _In_ otai_common_api_t api);

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
            otai_status_t waitForGetResponse(
                    _In_ otai_object_type_t objectType,
                    _In_ uint32_t attr_count,
                    _Inout_ otai_attribute_t *attr_list);

        private: // stats API response

            otai_status_t waitForGetStatsResponse(
                    _In_ otai_object_type_t object_type,
                    _In_ uint32_t number_of_counters,
                    _In_ const otai_stat_id_t *counter_ids,
                    _Out_ otai_stat_value_t *counters);

            otai_status_t waitForClearStatsResponse();

        private: // notification
            void handleNotification(
                    _In_ const std::string &name,
                    _In_ const std::string &serializedNotification,
                    _In_ const std::vector<swss::FieldValueTuple> &values);

            otai_status_t setRedisExtensionAttribute(
                    _In_ otai_object_type_t objectType,
                    _In_ otai_object_id_t objectId,
                    _In_ const otai_attribute_t *attr);

        private:
            void clear_local_state();

            std::string getHardwareInfo(
                    _In_ uint32_t attrCount,
                    _In_ const otai_attribute_t *attrList) const;

        private:

            std::shared_ptr<ContextConfig> m_contextConfig;

            bool m_initialized;

            std::shared_ptr<LinecardContainer> m_linecardContainer;

            std::shared_ptr<VirtualObjectIdManager> m_virtualObjectIdManager;

            std::shared_ptr<swss::DBConnector> m_db;

            std::shared_ptr<RedisVidIndexGenerator> m_redisVidIndexGenerator;

            std::weak_ptr<otaimeta::Meta> m_meta;

            std::shared_ptr<Channel> m_communicationChannel;

            std::function<otai_linecard_notifications_t(std::shared_ptr<Notification>)> m_notificationCallback;

    };
}
