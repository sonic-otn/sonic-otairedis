#pragma once
#include <thread>
#include "LinecardState.h"
#include "LinecardConfig.h"
#include "RealObjectIdManager.h"
#include "EventPayloadNetLinkMsg.h"

#include <set>
#include <unordered_set>
#include <vector>

#define DEFAULT_VLAN_NUMBER 1

#define MAX_OBJLIST_LEN 128

#define CHECK_STATUS(status) {                                  \
    otai_status_t _status = (status);                            \
    if (_status != OTAI_STATUS_SUCCESS) { return _status; } }

namespace otaivs
{
    class LinecardStateBase:
        public LinecardState
    {
        public:

            typedef std::map<otai_object_id_t, std::shared_ptr<LinecardStateBase>> LinecardStateMap;

        public:

            LinecardStateBase(
                    _In_ otai_object_id_t linecard_id,
                    _In_ std::shared_ptr<RealObjectIdManager> manager,
                    _In_ std::shared_ptr<LinecardConfig> config);

            virtual ~LinecardStateBase();

        protected:

            virtual otai_status_t set_linecard_default_attributes();

        public:

            virtual otai_status_t initialize_default_objects(
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

        public:

            virtual otai_status_t refresh_read_only(
                    _In_ const otai_attr_metadata_t *meta,
                    _In_ otai_object_id_t object_id);

        protected: // will generate new OID

            virtual otai_status_t create(
                    _In_ otai_object_type_t object_type,
                    _Out_ otai_object_id_t *object_id,
                    _In_ otai_object_id_t linecard_id,
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

            virtual otai_status_t remove(
                    _In_ otai_object_type_t object_type,
                    _In_ otai_object_id_t object_id);

            virtual otai_status_t set(
                    _In_ otai_object_type_t objectType,
                    _In_ otai_object_id_t objectId,
                    _In_ const otai_attribute_t* attr);

            virtual otai_status_t get(
                    _In_ otai_object_type_t object_type,
                    _In_ otai_object_id_t object_id,
                    _In_ uint32_t attr_count,
                    _Out_ otai_attribute_t *attr_list);

        public:

            virtual otai_status_t create(
                    _In_ otai_object_type_t object_type,
                    _In_ const std::string &serializedObjectId,
                    _In_ otai_object_id_t linecard_id,
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

            virtual otai_status_t remove(
                    _In_ otai_object_type_t object_type,
                    _In_ const std::string &serializedObjectId);

            virtual otai_status_t set(
                    _In_ otai_object_type_t objectType,
                    _In_ const std::string &serializedObjectId,
                    _In_ const otai_attribute_t* attr);

            virtual otai_status_t get(
                    _In_ otai_object_type_t object_type,
                    _In_ const std::string &serializedObjectId,
                    _In_ uint32_t attr_count,
                    _Out_ otai_attribute_t *attr_list);

        protected:
             void setObjectHash(
                     _In_ otai_object_type_t object_type,
                     _In_ const std::string &serializedObjectId,
                     _In_ const otai_attribute_t *attr);

            virtual otai_status_t remove_internal(
                    _In_ otai_object_type_t object_type,
                    _In_ const std::string &serializedObjectId);

            virtual otai_status_t create_internal(
                    _In_ otai_object_type_t object_type,
                    _In_ const std::string &serializedObjectId,
                    _In_ otai_object_id_t linecard_id,
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

            virtual otai_status_t set_internal(
                    _In_ otai_object_type_t objectType,
                    _In_ const std::string &serializedObjectId,
                    _In_ const otai_attribute_t* attr);

        private:

            otai_object_type_t objectTypeQuery(
                    _In_ otai_object_id_t objectId);

            otai_object_id_t linecardIdQuery(
                    _In_ otai_object_id_t objectId);

            bool check_object_default_state(
                    _In_ otai_object_id_t object_id);

            bool check_object_list_default_state(
                    _Out_ const std::vector<otai_object_id_t>& objlist);

        public:

            static int promisc(
                    _In_ const char *dev);

        protected: // custom linecard

            void send_linecard_state_change_notification(
                    _In_ otai_object_id_t linecard_id,
                    _In_ otai_oper_status_t status,
                    _In_ bool force);

            void send_linecard_alarm_notification(
                    _In_ otai_object_id_t linecard_id,
                    _In_ otai_alarm_type_t alarm_type,
                    _In_ otai_alarm_info_t alarm_info);

            void send_ocm_spectrum_power_notification(
                    _In_ otai_object_id_t linecard_id,
                    _In_ otai_object_id_t ocm_id);

            void send_otdr_result_notification(
                    _In_ otai_object_id_t linecard_id,
                    _In_ otai_object_id_t otdr_id);

            void send_aps_switch_info_notification(
                _In_ otai_object_id_t linecard_id);

        public: // TODO move inside warm boot load state

            void syncOnLinkMsg(
                    _In_ std::shared_ptr<EventPayloadNetLinkMsg> payload);

        protected:

            void findObjects(
                    _In_ otai_object_type_t object_type,
                    _In_ const otai_attribute_t &expect,
                    _Out_ std::vector<otai_object_id_t> &objects);

            bool dumpObject(
                    _In_ const otai_object_id_t object_id,
                    _Out_ std::vector<otai_attribute_t> &attrs);

        protected:

            constexpr static const int maxDebugCounters = 32;

            std::unordered_set<uint32_t> m_indices;


        public: // TODO private

            std::shared_ptr<RealObjectIdManager> m_realObjectIdManager;

            std::shared_ptr<std::thread> m_externEventThread;

            void externEventThreadProc();

            bool m_externEventThreadRun;

        public: // TODO private

            std::shared_ptr<std::thread> m_updateObjectThread;

            void updateObjectThreadProc();

            bool m_updateObjectThreadRun;
   };
}

