#pragma once

#include <memory>
#include <mutex>

#include "CommandLineOptions.h"
#include "FlexCounterManager.h"
#include "VendorLai.h"
#include "LaiLinecard.h"
#include "VirtualOidTranslator.h"
#include "RedisClient.h"
#include "NotificationHandler.h"
#include "NotificationProcessor.h"
#include "LinecardNotifications.h"
#include "ServiceMethodTable.h"
#include "RedisVidIndexGenerator.h"
#include "RequestShutdown.h"
#include "ContextConfig.h"
#include "NotificationProducerBase.h"
#include "SelectableChannel.h"

#include "meta/LaiAttributeList.h"

#include "swss/consumertable.h"
#include "swss/producertable.h"
#include "swss/notificationconsumer.h"

#include "swss/dbconnector.h"
#include "swss/select.h"
#include "swss/selectableevent.h"
#include "swss/table.h"
#include "swss/subscriberstatetable.h"


namespace syncd
{
    extern int64_t time_zone_nanosecs;
    class Syncd
    {
    private:

        Syncd(const Syncd&) = delete;
        Syncd& operator=(const Syncd&) = delete;

    public:

        Syncd(
            _In_ std::shared_ptr<lairedis::LaiInterface> vendorLai,
            _In_ std::shared_ptr<CommandLineOptions> cmd,
            _In_ bool needCheckLink);

        virtual ~Syncd();

    public:

        bool isVeryFirstRun();

        void onSyncdStart();

        void run();

    public: // TODO private

        void processEvent(
            _In_ SelectableChannel& consumer);

        void processFlexCounterGroupEvent(
            _In_ swss::ConsumerTable& consumer);

        void processFlexCounterEvent(
            _In_ swss::ConsumerTable& consumer);

        const char* profileGetValue(
            _In_ lai_linecard_profile_id_t profile_id,
            _In_ const char* variable);

        int profileGetNextValue(
            _In_ lai_linecard_profile_id_t profile_id,
            _Out_ const char** variable,
            _Out_ const char** value);

        void sendShutdownRequestAfterException();

    public: // shutdown actions for all linecards

        lai_status_t removeAllLinecards();

    private:

        void loadProfileMap();

        void laiLoglevelNotify(
            _In_ std::string strApi,
            _In_ std::string strLogLevel);

        void setLaiApiLogLevel();

    private:

        lai_status_t processSingleEvent(
            _In_ const swss::KeyOpFieldsValuesTuple& kco);

        lai_status_t processAttrCapabilityQuery(
            _In_ const swss::KeyOpFieldsValuesTuple& kco);

        lai_status_t processAttrEnumValuesCapabilityQuery(
            _In_ const swss::KeyOpFieldsValuesTuple& kco);

        lai_status_t processObjectTypeGetAvailabilityQuery(
            _In_ const swss::KeyOpFieldsValuesTuple& kco);

        lai_status_t processClearStatsEvent(
            _In_ const swss::KeyOpFieldsValuesTuple& kco);

        lai_status_t processGetStatsEvent(
            _In_ const swss::KeyOpFieldsValuesTuple& kco);

        lai_status_t processQuadEvent(
            _In_ lai_common_api_t api,
            _In_ const swss::KeyOpFieldsValuesTuple& kco);

        lai_status_t processBulkQuadEvent(
            _In_ lai_common_api_t api,
            _In_ const swss::KeyOpFieldsValuesTuple& kco);

        lai_status_t processBulkOid(
            _In_ lai_object_type_t objectType,
            _In_ const std::vector<std::string>& object_ids,
            _In_ lai_common_api_t api,
            _In_ const std::vector<std::shared_ptr<laimeta::LaiAttributeList>>& attributes,
            _In_ const std::vector<std::vector<swss::FieldValueTuple>>& strAttributes);

        lai_status_t processBulkEntry(
            _In_ lai_object_type_t objectType,
            _In_ const std::vector<std::string>& object_ids,
            _In_ lai_common_api_t api,
            _In_ const std::vector<std::shared_ptr<laimeta::LaiAttributeList>>& attributes,
            _In_ const std::vector<std::vector<swss::FieldValueTuple>>& strAttributes);

        lai_status_t processBulkCreateEntry(
            _In_ lai_object_type_t objectType,
            _In_ const std::vector<std::string>& objectIds,
            _In_ const std::vector<std::shared_ptr<laimeta::LaiAttributeList>>& attributes,
            _Out_ std::vector<lai_status_t>& statuses);

        lai_status_t processBulkRemoveEntry(
            _In_ lai_object_type_t objectType,
            _In_ const std::vector<std::string>& objectIds,
            _Out_ std::vector<lai_status_t>& statuses);

        lai_status_t processBulkSetEntry(
            _In_ lai_object_type_t objectType,
            _In_ const std::vector<std::string>& objectIds,
            _In_ const std::vector<std::shared_ptr<laimeta::LaiAttributeList>>& attributes,
            _Out_ std::vector<lai_status_t>& statuses);

        lai_status_t processOid(
            _In_ lai_object_type_t objectType,
            _In_ const std::string& strObjectId,
            _In_ lai_common_api_t api,
            _In_ uint32_t attr_count,
            _In_ lai_attribute_t* attr_list);

    private: // process quad oid

        lai_status_t processOidCreate(
            _In_ lai_object_type_t objectType,
            _In_ const std::string& strObjectId,
            _In_ uint32_t attr_count,
            _In_ lai_attribute_t* attr_list);

        lai_status_t processOidRemove(
            _In_ lai_object_type_t objectType,
            _In_ const std::string& strObjectId);

        lai_status_t processOidSet(
            _In_ lai_object_type_t objectType,
            _In_ const std::string& strObjectId,
            _In_ lai_attribute_t* attr);

        lai_status_t processOidGet(
            _In_ lai_object_type_t objectType,
            _In_ const std::string& strObjectId,
            _In_ uint32_t attr_count,
            _In_ lai_attribute_t* attr_list);

    private: // process bulk oid

        lai_status_t processBulkOidCreate(
            _In_ lai_object_type_t objectType,
            _In_ lai_bulk_op_error_mode_t mode,
            _In_ const std::vector<std::string>& objectIds,
            _In_ const std::vector<std::shared_ptr<laimeta::LaiAttributeList>>& attributes,
            _Out_ std::vector<lai_status_t>& statuses);

        lai_status_t processBulkOidRemove(
            _In_ lai_object_type_t objectType,
            _In_ lai_bulk_op_error_mode_t mode,
            _In_ const std::vector<std::string>& objectIds,
            _Out_ std::vector<lai_status_t>& statuses);

    private:

        void syncUpdateRedisQuadEvent(
            _In_ lai_status_t status,
            _In_ lai_common_api_t api,
            _In_ const swss::KeyOpFieldsValuesTuple& kco);

        void syncUpdateRedisBulkQuadEvent(
            _In_ lai_common_api_t api,
            _In_ const std::vector<lai_status_t>& statuses,
            _In_ lai_object_type_t objectType,
            _In_ const std::vector<std::string>& objectIds,
            _In_ const std::vector<std::vector<swss::FieldValueTuple>>& strAttributes);

    public: // TODO to private

        lai_status_t processEntry(
            _In_ lai_object_meta_key_t meta_key,
            _In_ lai_common_api_t api,
            _In_ uint32_t attr_count,
            _In_ lai_attribute_t* attr_list);

        void syncProcessNotification(
            _In_ const swss::KeyOpFieldsValuesTuple& item);

        void handleLinecardStateChange(
            _In_ const lai_oper_status_t& linecard_oper_status);

    private:

        syncd_restart_type_t handleRestartQuery(
            _In_ swss::NotificationConsumer& restartQuery);

        lai_oper_status_t handleLinecardState(
            _In_ swss::NotificationConsumer& linecardState);

        void preprocessOidOps(lai_object_type_t objectType, lai_attribute_t* attr_list, uint32_t attr_count);

    private:

        /**
         * @brief Send api response.
         *
         * This function should be use to send response to lairedis for
         * create/remove/set API as well as their corresponding bulk versions.
         *
         * Should not be used on GET api.
         */
        void sendApiResponse(
            _In_ lai_common_api_t api,
            _In_ lai_status_t status,
            _In_ uint32_t object_count = 0,
            _In_ lai_status_t* object_statuses = NULL);

        void sendGetResponse(
            _In_ lai_object_type_t objectType,
            _In_ const std::string& strObjectId,
            _In_ lai_object_id_t linecardVid,
            _In_ lai_status_t status,
            _In_ uint32_t attr_count,
            _In_ lai_attribute_t* attr_list);

    private:

        std::shared_ptr<CommandLineOptions> m_commandLineOptions;


        bool m_linkCheckLoop;

        LinecardNotifications m_ln;

        ServiceMethodTable m_smt;

        lai_service_method_table_t m_test_services;

    public: // TODO to private

        std::shared_ptr<FlexCounterManager> m_manager;

        /**
         * @brief set of objects removed by user when we are in init view
         * mode. Those could be vlan members, bridge ports etc.
         *
         * We need this list to later on not put them back to temp view
         * mode when doing populate existing objects in apply view mode.
         *
         * Object ids here a VIDs.
         */
        std::set<lai_object_id_t> m_initViewRemovedVidSet;

        std::shared_ptr<lairedis::LaiInterface> m_vendorLai;

        /*
         * TODO: Those are hard coded values for mlnx integration for v1.0.1 they need
         * to be updated.
         *
         * Also DEVICE_MAC_ADDRESS is not present in lailinecard.h
         */
        std::map<std::string, std::string> m_profileMap;

        std::map<std::string, std::string>::iterator m_profileIter;


        /**
         * @brief Contains map of all created linecards.
         *
         * This syncd implementation supports only one linecard but it's
         * written in a way that could be extended to use multiple linecards
         * in the future, some refactoring needs to be made in marked
         * places.
         *
         * To support multiple linecards VIDTORID and RIDTOVID db entries
         * needs to be made per linecard like HIDDEN and LANES. Best way is
         * to wrap vid/rid map to functions that will return right key.
         *
         * Key is linecard VID.
         */
        std::map<lai_object_id_t, std::shared_ptr<syncd::LaiLinecard>> m_linecards;

        std::shared_ptr<VirtualOidTranslator> m_translator;

        std::shared_ptr<RedisClient> m_client;

        std::shared_ptr<NotificationHandler> m_handler;

        std::shared_ptr<syncd::NotificationProcessor> m_processor;

        std::shared_ptr<SelectableChannel> m_selectableChannel;

        bool m_enableSyncMode;

    private:

        /**
         * @brief Syncd mutex for thread synchronization
         *
         * Purpose of this mutex is to synchronize multiple threads like
         * main thread, counters and notifications as well as all
         * operations which require multiple Redis DB access.
         *
         * For example: query DB for next VID id number, and then put map
         * RID and VID to Redis. From syncd point of view this entire
         * operation should be atomic and no other thread should access DB
         * or make assumption on previous information until entire
         * operation will finish.
         *
         * Mutex must be used in 4 places:
         *
         * - notification processing
         * - main event loop processing
         * - syncd hard init when linecards are created
         *   (notifications could be sent during that)
         * - in case of exception when sending shutdown request
         *   (other notifications can still arrive at this point)
         *
         * * getting flex counter - here we skip using mutex
         */
        std::mutex m_mutex;

        std::shared_ptr<swss::DBConnector> m_dbAsic;

        std::shared_ptr<swss::NotificationConsumer> m_restartQuery;
        std::shared_ptr<swss::NotificationConsumer> m_linecardStateNtf;

        std::shared_ptr<swss::DBConnector> m_dbFlexCounter;
        std::shared_ptr<swss::ConsumerTable> m_flexCounter;
        std::shared_ptr<swss::ConsumerTable> m_flexCounterGroup;

        std::shared_ptr<NotificationProducerBase> m_notifications;

        std::shared_ptr<lairedis::LinecardConfigContainer> m_linecardConfigContainer;
        std::shared_ptr<lairedis::RedisVidIndexGenerator> m_redisVidIndexGenerator;
        std::shared_ptr<lairedis::VirtualObjectIdManager> m_virtualObjectIdManager;

        std::shared_ptr<lairedis::ContextConfig> m_contextConfig;

        std::shared_ptr<swss::DBConnector> m_state_db;
        std::unique_ptr<swss::Table> m_linecardtable;

        std::mutex m_mtxAlarmTable;
        std::unique_ptr<swss::Table> m_curalarmtable;
        lai_oper_status_t m_linecardState;
    };
}
