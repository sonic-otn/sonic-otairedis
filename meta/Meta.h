#pragma once

#include "OtaiInterface.h"

#include "OtaiAttrWrapper.h"
#include "OtaiObjectCollection.h"
#include "AttrKeyMap.h"
#include "OidRefCounter.h"

#include "swss/table.h"

#include <vector>
#include <memory>
#include <set>

namespace otaimeta
{
    class Meta:
        public otairedis::OtaiInterface
    {
        public:

            using otairedis::OtaiInterface::set; // name hiding

            Meta(
                    _In_ std::shared_ptr<OtaiInterface> impl);

            virtual ~Meta() = default;

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

            void meta_init_db();

            bool isEmpty();

        public: // notifications

            void meta_otai_on_linecard_state_change(
                    _In_ otai_object_id_t linecard_id,
                    _In_ otai_oper_status_t linecard_oper_status);

            void meta_otai_on_linecard_shutdown_request(
                    _In_ otai_object_id_t linecard_id);

        private: // validation helpers

            otai_status_t meta_generic_validation_objlist(
                    _In_ const otai_attr_metadata_t& md,
                    _In_ otai_object_id_t linecard_id,
                    _In_ uint32_t count,
                    _In_ const otai_object_id_t* list);

            otai_status_t meta_genetic_validation_list(
                    _In_ const otai_attr_metadata_t& md,
                    _In_ uint32_t count,
                    _In_ const void* list);

            otai_status_t meta_generic_validate_non_object_on_create(
                    _In_ const otai_object_meta_key_t& meta_key,
                    _In_ otai_object_id_t linecard_id);

            otai_object_id_t meta_extract_linecard_id(
                    _In_ const otai_object_meta_key_t& meta_key,
                    _In_ otai_object_id_t linecard_id);

            std::shared_ptr<OtaiAttrWrapper> get_object_previous_attr(
                    _In_ const otai_object_meta_key_t& metaKey,
                    _In_ const otai_attr_metadata_t& md);

            std::vector<const otai_attr_metadata_t*> get_attributes_metadata(
                    _In_ otai_object_type_t objecttype);

            void meta_generic_validation_post_get_objlist(
                    _In_ const otai_object_meta_key_t& meta_key,
                    _In_ const otai_attr_metadata_t& md,
                    _In_ otai_object_id_t linecard_id,
                    _In_ uint32_t count,
                    _In_ const otai_object_id_t* list);

        public: 

            int32_t getObjectReferenceCount(
                    _In_ otai_object_id_t oid) const;

            bool objectExists(
                    _In_ const otai_object_meta_key_t& mk) const;

        public: // validation post QUAD

            void meta_generic_validation_post_create(
                    _In_ const otai_object_meta_key_t& meta_key,
                    _In_ otai_object_id_t linecard_id,
                    _In_ const uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

            void meta_generic_validation_post_remove(
                    _In_ const otai_object_meta_key_t& meta_key);

            void meta_generic_validation_post_set(
                    _In_ const otai_object_meta_key_t& meta_key,
                    _In_ const otai_attribute_t *attr);

            void meta_generic_validation_post_get(
                    _In_ const otai_object_meta_key_t& meta_key,
                    _In_ otai_object_id_t linecard_id,
                    _In_ const uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

        private: // validation QUAD

            otai_status_t meta_generic_validation_create(
                    _In_ const otai_object_meta_key_t& meta_key,
                    _In_ otai_object_id_t linecard_id,
                    _In_ const uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

            otai_status_t meta_generic_validation_remove(
                    _In_ const otai_object_meta_key_t& meta_key);

            otai_status_t meta_generic_validation_set(
                    _In_ const otai_object_meta_key_t& meta_key,
                    _In_ const otai_attribute_t *attr);

            otai_status_t meta_generic_validation_get(
                    _In_ const otai_object_meta_key_t& meta_key,
                    _In_ const uint32_t attr_count,
                    _In_ otai_attribute_t *attr_list);

        private: // stats

            otai_status_t meta_validate_stats(
                    _In_ otai_object_type_t object_type,
                    _In_ otai_object_id_t object_id,
                    _In_ uint32_t number_of_counters,
                    _In_ const otai_stat_id_t *counter_ids,
                    _Out_ otai_stat_value_t *counters,
                    _In_ otai_stats_mode_t mode);

        private: // validate OID

            otai_status_t meta_otai_validate_oid(
                    _In_ otai_object_type_t object_type,
                    _In_ const otai_object_id_t* object_id,
                    _In_ otai_object_id_t linecard_id,
                    _In_ bool create);

        private:

            void clean_after_linecard_remove(
                    _In_ otai_object_id_t linecardId);

        private:

            std::shared_ptr<otairedis::OtaiInterface> m_implementation;

        private: // database objects
            /*
             * Non object ids don't need reference count since they are leafs and can be
             * removed at any time.
             */

            OidRefCounter m_oids;

            OtaiObjectCollection m_otaiObjectCollection;

            AttrKeyMap m_attrKeys;

        private: // warm boot

            bool m_warmBoot;
    };
}


