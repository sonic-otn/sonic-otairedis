#pragma once

#include "lib/inc/LaiInterface.h"

#include <memory>

namespace laimeta
{
    /**
     * @brief Dummy SAI interface.
     *
     * Defined for unittests.
     */
    class DummyLaiInterface:
        public lairedis::LaiInterface
    {
        public:

            DummyLaiInterface();

            virtual ~DummyLaiInterface() = default;

        public:

            void setStatus(
                    _In_ lai_status_t status);

        public:

            virtual lai_status_t initialize(
                    _In_ uint64_t flags,
                    _In_ const lai_service_method_table_t *service_method_table) override;

            virtual lai_status_t uninitialize(void) override;

            virtual lai_status_t linkCheck(_Out_ bool *up) override;

        public: // SAI interface overrides

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

        public: // SAI API

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

        protected:

            lai_status_t m_status;
    };
}
