#pragma once

#include "lib/inc/OtaiInterface.h"

#include "OtaiAttrWrapper.h"
#include "OtaiObjectCollection.h"
#include "PortRelatedSet.h"
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

            virtual otai_status_t objectTypeGetAvailability(
                    _In_ otai_object_id_t linecardId,
                    _In_ otai_object_type_t objectType,
                    _In_ uint32_t attrCount,
                    _In_ const otai_attribute_t *attrList,
                    _Out_ uint64_t *count) override;

            virtual otai_status_t queryAttributeCapability(
                    _In_ otai_object_id_t linecard_id,
                    _In_ otai_object_type_t object_type,
                    _In_ otai_attr_id_t attr_id,
                    _Out_ otai_attr_capability_t *capability) override;

            virtual otai_status_t queryAattributeEnumValuesCapability(
                    _In_ otai_object_id_t linecard_id,
                    _In_ otai_object_type_t object_type,
                    _In_ otai_attr_id_t attr_id,
                    _Inout_ otai_s32_list_t *enum_values_capability) override;

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

            static bool is_ipv6_mask_valid(
                    _In_ const uint8_t* mask);

        private: // unit tests helpers

            bool meta_unittests_get_and_erase_set_readonly_flag(
                    _In_ const otai_attr_metadata_t& md);

        public:

            /**
             * @brief Enable unittest globally.
             *
             * @param[in] enable If set to true unittests are enabled.
             */
            void meta_unittests_enable(
                    _In_ bool enable);

            /**
             * @brief Indicates whether unittests are enabled;
             */
            bool meta_unittests_enabled();


            /**
             * @brief Allow to perform SET operation on READ_ONLY attribute only once.
             *
             * This function relaxes metadata checking on SET operation, it allows to
             * perform SET api on READ_ONLY attribute only once on specific object type and
             * specific attribute.
             *
             * Once means that SET operation is only relaxed for the very next SET call on
             * that specific object type and attribute id.
             *
             * Function is explicitly named ONCE, since it will force test developer to not
             * forget that SET check is relaxed, and not forget for future unittests.
             *
             * Function is provided for more flexible testing using virtual linecard.  Since
             * some of the read only attributes maybe very complex to simulate (for example
             * resources used by actual asic when adding next hop or next hop group), then
             * it's easier to write such unittest:
             *
             * TestCase:
             * 1. meta_unittests_allow_readonly_set_once(x,y);
             * 2. object_x_api->set_attribute(object_id, attr, foo); // attr.id == y
             * 3. object_x_api->get_attribute(object_id, 1, attr); // attr.id == y
             * 4. check if get result is equal to set result.
             *
             * On real ASIC, even after allowing SET on read only attribute, actual SET
             * should fail.
             *
             * It can be dangerous to set any readonly attribute to different values since
             * internal metadata logic maybe using that value and in some cases metadata
             * database may get out of sync and cause unexpected results in api calls up to
             * application crash.
             *
             * This function is not thread safe.
             *
             * @param[in] object_type Object type on which SET will be possible.
             * @param[in] attr_id Attribute ID on which SET will be possible.
             *
             * @return #OTAI_STATUS_SUCCESS on success Failure status code on error
             */
            otai_status_t meta_unittests_allow_readonly_set_once(
                    _In_ otai_object_type_t object_type,
                    _In_ int32_t attr_id);

        public: // unittests method helpers

            int32_t getObjectReferenceCount(
                    _In_ otai_object_id_t oid) const;

            bool objectExists(
                    _In_ const otai_object_meta_key_t& mk) const;

        private: // port helpers

            otai_status_t meta_port_remove_validation(
                    _In_ const otai_object_meta_key_t& meta_key);

            bool meta_is_object_in_default_state(
                    _In_ otai_object_id_t oid);

            void post_port_remove(
                    _In_ const otai_object_meta_key_t& meta_key);

            void meta_post_port_get(
                    _In_ const otai_object_meta_key_t& meta_key,
                    _In_ otai_object_id_t linecard_id,
                    _In_ const uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

            void meta_add_port_to_related_map(
                    _In_ otai_object_id_t port_id,
                    _In_ const otai_object_list_t& list);

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


            /**
             * @brief Port related objects set.
             *
             * Key in map is port OID, and value is set of related objects ids
             * like queues, ipgs and scheduler groups.
             *
             * This map will help to identify objects to be automatically removed
             * when port will be removed.
             */
            PortRelatedSet m_portRelatedSet;

            /*
             * Non object ids don't need reference count since they are leafs and can be
             * removed at any time.
             */

            OidRefCounter m_oids;

            OtaiObjectCollection m_otaiObjectCollection;

            AttrKeyMap m_attrKeys;

        private: // unittests

            std::set<std::string> m_meta_unittests_set_readonly_set;

            bool m_unittestsEnabled;

        private: // warm boot

            bool m_warmBoot;
    };
}


