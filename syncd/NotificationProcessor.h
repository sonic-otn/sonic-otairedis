#pragma once

#include <mutex>
#include <thread>
#include <memory>
#include <condition_variable>
#include <functional>
#include <queue>

#include "lairediscommon.h"
#include "NotificationQueue.h"
#include "VirtualOidTranslator.h"
#include "RedisClient.h"
#include "NotificationProducerBase.h"

#include "swss/notificationproducer.h"

namespace syncd
{
    class NotificationProcessor
    {
    public:

        NotificationProcessor(
            _In_ std::mutex& mtxAlarm,
            _In_ std::shared_ptr<NotificationProducerBase> producer,
            _In_ std::string dbAsic,
            _In_ std::shared_ptr<RedisClient> client,
            _In_ std::function<void(const swss::KeyOpFieldsValuesTuple&)> synchronizer,
            _In_ std::function<void(const lai_oper_status_t&)> linecard_state_change_handler);

        virtual ~NotificationProcessor();

    public:

        std::shared_ptr<NotificationQueue> getQueue() const;

        void signal();

        void startNotificationsProcessingThread();

        void stopNotificationsProcessingThread();

    public:

        void ntf_process_function();

        void sendNotification(
            _In_ const std::string& op,
            _In_ const std::string& data,
            _In_ std::vector<swss::FieldValueTuple> entry);

        void sendNotification(
            _In_ const std::string& op,
            _In_ const std::string& data);

    private: // processors

        void process_on_linecard_state_change(
            _In_ lai_object_id_t linecard_rid,
            _In_ lai_oper_status_t linecard_oper_status);

    private: // handlers

        void handle_linecard_state_change(
            _In_ const std::string& data);

        void handle_olp_switch_notify(
            _In_ const std::string& data,
            _In_ const std::vector<swss::FieldValueTuple>& fv);

        void handle_ocm_spectrum_power_notify(
            _In_ const std::string& data,
            _In_ const std::vector<swss::FieldValueTuple>& fv);

        void handle_otdr_result_notify(
            _In_ const std::string& data,
            _In_ const std::vector<swss::FieldValueTuple>& fv);

        std::string get_resource_name_by_rid(
            _In_ lai_object_id_t rid);

        void handle_linecard_alarm(
            _In_ const std::string& data);

        void handler_alarm_generated(
            _In_ const std::string data);

        void handler_alarm_cleared(
            _In_ const std::string data);

        void handler_event_generated(
            _In_ const std::string data);

        void handler_history_alarm(
            _In_ const std::string& key,
            _In_ const std::string& timecreated,
            _In_ const std::vector<swss::FieldValueTuple>& alarmvector);

        void processNotification(
            _In_ const swss::KeyOpFieldsValuesTuple& item);

        void initOtdrScanTimeSet();

    public:

        void syncProcessNotification(
            _In_ const swss::KeyOpFieldsValuesTuple& item);

    public: // TODO to private

        std::shared_ptr<VirtualOidTranslator> m_translator;

    private:

        std::shared_ptr<NotificationQueue> m_notificationQueue;

        std::shared_ptr<std::thread> m_ntf_process_thread;

        // condition variable will be used to notify processing thread
        // that some notification arrived

        std::condition_variable m_cv;

        // determine whether notification thread is running

        bool m_runThread;
        std::mutex& m_mtxAlarmTable;

        std::function<void(const swss::KeyOpFieldsValuesTuple&)> m_synchronizer;
        std::function<void(const lai_oper_status_t&)> m_linecard_state_change_handler;

        std::shared_ptr<RedisClient> m_client;

        std::shared_ptr<NotificationProducerBase> m_notifications;
        std::string m_dbAsic;
        std::shared_ptr<swss::DBConnector> m_state_db;

        std::unique_ptr<swss::Table> m_stateAlarmable;
        std::unique_ptr<swss::Table> m_stateOLPSwitchInfoTbl;
        std::unique_ptr<swss::Table> m_stateOcmTable;

        std::shared_ptr<swss::Table> m_stateOtdrTable;
        std::shared_ptr<swss::Table> m_stateOtdrEventTable;

        std::shared_ptr<swss::DBConnector> m_history_db;
        std::unique_ptr<swss::Table> m_historyAlarmTable;
        std::unique_ptr<swss::Table> m_historyEventTable;

        std::shared_ptr<swss::Table> m_historyOtdrTable;
        std::shared_ptr<swss::Table> m_historyOtdrEventTable;

        std::map<std::string, std::queue<uint64_t>> m_otdrScanTimeQueue;

        uint32_t m_ttlPM15Min;
        uint32_t m_ttlPM24Hour;
        uint32_t m_ttlAlarm;
    };
}
