#pragma once

extern "C" {
#include "otai.h"
}

namespace syncd
{
    class VidManager
    {

        private:

            VidManager() = delete;

            ~VidManager() = delete;

        public:

            /**
             * @brief Linecard id query.
             *
             * Return linecard object id for given object if. If object type is
             * linecard, it will return input value.
             *
             * For OTAI_NULL_OBJECT_ID returns OTAI_NULL_OBJECT_ID.
             *
             * Throws for invalid object ID.
             */
            static otai_object_id_t linecardIdQuery(
                    _In_ otai_object_id_t objectId);

            /**
             * @brief Object type query.
             *
             * Returns object type for input object id. If object id is invalid
             * then returns OTAI_OBJECT_TYPE_NULL.
             *
             * For OTAI_NULL_OBJECT_ID returns OTAI_OBJECT_TYPE_NULL.
             *
             * Throws for invalid object ID.
             */
            static otai_object_type_t objectTypeQuery(
                    _In_ otai_object_id_t objectId);

            /**
             * @brief Get linecard index.
             *
             * Index range is <0..255>.
             *
             * Returns linecard index for given oid. 
             *
             * For OTAI_NULL_OBJECT_ID returns 0.
             *
             * Throws for invalid object ID.
             */
            static uint32_t getLinecardIndex(
                    _In_ otai_object_id_t objectId);

            /**
             * @brief Get object index.
             *
             * Returns object index.
             */
            static uint64_t getObjectIndex(
                    _In_ otai_object_id_t objectId);

            /**
             * @brief Update object index.
             *
             * Returns objects with updated object index.
             */
            static otai_object_id_t updateObjectIndex(
                    _In_ otai_object_id_t objectId,
                    _In_ uint64_t objectIndex);
    };
}
