#pragma once

#include "LinecardStateBase.h"
#include "LinecardConfigContainer.h"
#include "RealObjectIdManager.h"
#include "LinecardStateBase.h"
#include "EventQueue.h"

#include "meta/OtaiInterface.h"

#include "meta/Meta.h"

#include <string>
#include <vector>
#include <map>

namespace otaivs
{
    class VirtualLinecardOtaiInterface:
        public otairedis::OtaiInterface
    {
        public:

            VirtualLinecardOtaiInterface(
                    _In_ const std::shared_ptr<LinecardConfigContainer> scc);

            virtual ~VirtualLinecardOtaiInterface();

        public:

            virtual otai_status_t initialize(
                    _In_ uint64_t flags,
                    _In_ const otai_service_method_table_t *service_method_table) override;

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

        private: // QUAD API helpers

            otai_status_t create(
                    _In_ otai_object_id_t linecardId,
                    _In_ otai_object_type_t objectType,
                    _In_ const std::string& serializedObjectId,
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

            otai_status_t remove(
                    _In_ otai_object_id_t linecardId,
                    _In_ otai_object_type_t objectType,
                    _In_ const std::string& serializedObjectId);

            otai_status_t set(
                    _In_ otai_object_id_t linecardId,
                    _In_ otai_object_type_t objectType,
                    _In_ const std::string& serializedObjectId,
                    _In_ const otai_attribute_t *attr);

            otai_status_t get(
                    _In_ otai_object_id_t linecardId,
                    _In_ otai_object_type_t objectType,
                    _In_ const std::string& serializedObjectId,
                    _In_ uint32_t attr_count,
                    _Inout_ otai_attribute_t *attr_list);

        private: // QUAD pre

            otai_status_t preSet(
                    _In_ otai_object_type_t objectType,
                    _In_ otai_object_id_t objectId,
                    _In_ const otai_attribute_t *attr);

            otai_status_t preSetPort(
                    _In_ otai_object_id_t objectId,
                    _In_ const otai_attribute_t *attr);

        private:

            std::shared_ptr<LinecardStateBase> init_linecard(
                    _In_ otai_object_id_t linecard_id,
                    _In_ std::shared_ptr<LinecardConfig> config,
                    _In_ std::weak_ptr<otaimeta::Meta> meta,
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

            void removeLinecard(
                    _In_ otai_object_id_t linecardId);

        public:

            void setMeta(
                    _In_ std::weak_ptr<otaimeta::Meta> meta);

            void ageFdbs();


        public:

            std::string getHardwareInfo(
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list) const;

        private:

            std::weak_ptr<otaimeta::Meta> m_meta;

            std::shared_ptr<LinecardConfigContainer> m_linecardConfigContainer;

            std::shared_ptr<RealObjectIdManager> m_realObjectIdManager;

            LinecardStateBase::LinecardStateMap m_linecardStateMap;
    };
}
