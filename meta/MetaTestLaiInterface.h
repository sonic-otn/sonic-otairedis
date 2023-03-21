#pragma once

#include "DummyLaiInterface.h"

#include "lib/inc/VirtualObjectIdManager.h"

#include <memory>

namespace laimeta
{
    class MetaTestLaiInterface:
        public DummyLaiInterface
    {
        public:

            MetaTestLaiInterface();

            virtual ~MetaTestLaiInterface() = default;

        public:

            virtual lai_status_t create(
                    _In_ lai_object_type_t objectType,
                    _Out_ lai_object_id_t* objectId,
                    _In_ lai_object_id_t linecardId,
                    _In_ uint32_t attr_count,
                    _In_ const lai_attribute_t *attr_list) override;

        public:

            virtual lai_object_type_t objectTypeQuery(
                    _In_ lai_object_id_t objectId) override;

            virtual lai_object_id_t linecardIdQuery(
                    _In_ lai_object_id_t objectId) override;

        private:

            std::shared_ptr<lairedis::VirtualObjectIdManager> m_virtualObjectIdManager;

    };
}
