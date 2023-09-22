#pragma once

#include "DummyOtaiInterface.h"

#include "lib/inc/VirtualObjectIdManager.h"

#include <memory>

namespace otaimeta
{
    class MetaTestOtaiInterface:
        public DummyOtaiInterface
    {
        public:

            MetaTestOtaiInterface();

            virtual ~MetaTestOtaiInterface() = default;

        public:

            virtual otai_status_t create(
                    _In_ otai_object_type_t objectType,
                    _Out_ otai_object_id_t* objectId,
                    _In_ otai_object_id_t linecardId,
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list) override;

        public:

            virtual otai_object_type_t objectTypeQuery(
                    _In_ otai_object_id_t objectId) override;

            virtual otai_object_id_t linecardIdQuery(
                    _In_ otai_object_id_t objectId) override;

        private:

            std::shared_ptr<otairedis::VirtualObjectIdManager> m_virtualObjectIdManager;

    };
}
