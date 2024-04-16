#pragma once

#include "lib/inc/OtaiInterface.h"

#include <memory>

namespace otaimeta
{
    /**
     * @brief Dummy SAI interface.
     *
     * Defined for unittests.
     */
    class DummyOtaiInterface:
        public otairedis::OtaiInterface
    {
        public:

            DummyOtaiInterface();

            virtual ~DummyOtaiInterface() = default;

        public:

            void setStatus(
                    _In_ otai_status_t status);

        public:

            virtual otai_status_t initialize(
                    _In_ uint64_t flags,
                    _In_ const otai_service_method_table_t *service_method_table) override;

            virtual otai_status_t uninitialize(void) override;

            virtual otai_status_t linkCheck(_Out_ bool *up) override;

        public: // SAI interface overrides

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

        public: // SAI API

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

        protected:

            otai_status_t m_status;
    };
}
