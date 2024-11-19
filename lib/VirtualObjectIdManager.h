#pragma once

extern "C" {
#include "otai.h"
}

#include "OidIndexGenerator.h"

#include <set>
#include <memory>

namespace otairedis
{
    class VirtualObjectIdManager
    {

        public:

            VirtualObjectIdManager(
                    _In_ std::shared_ptr<OidIndexGenerator> oidIndexGenerator);

            virtual ~VirtualObjectIdManager() = default;

        public:

            /**
             * @brief Linecard id query.
             *
             * Return linecard object id for given object if. If object type is
             * linecard, it will return input value.
             *
             * Throws if given object id has invalid object type.
             *
             * For OTAI_NULL_OBJECT_ID input will return OTAI_NULL_OBJECT_ID.
             */
            otai_object_id_t otaiLinecardIdQuery(
                    _In_ otai_object_id_t objectId) const;

            /**
             * @brief Object type query.
             *
             * Returns object type for input object id. If object id is invalid
             * then returns OTAI_OBJECT_TYPE_NULL.
             */
            otai_object_type_t otaiObjectTypeQuery(
                    _In_ otai_object_id_t objectId) const;

            /**
             * @brief Clear linecard index set. 
             *
             * New linecard index allocation will start from the beginning.
             */
            void clear();

            /**
             * @brief Allocate new object id on a given linecard.
             *
             * If object type is linecard, then linecard id param is ignored.
             *
             * Throws when object type is linecard and there are no more
             * available linecard indexes.
             */
            otai_object_id_t allocateNewObjectId(
                    _In_ otai_object_type_t objectType,
                    _In_ otai_object_id_t linecardId);

            /**
             * @brief Allocate new linecard object id.
             */
            otai_object_id_t allocateNewLinecardObjectId();

        private:
            /**
             * @brief Construct object id.
             *
             * Using all input parameters to construct object id.
             */
            static otai_object_id_t constructObjectId(
                    _In_ otai_object_type_t objectType,
                    _In_ uint32_t linecardIndex,
                    _In_ uint64_t objectIndex);

        public:

            /**
             * @brief Linecard id query.
             *
             * Return linecard object id for given object if. If object type is
             * linecard, it will return input value.
             *
             * Return OTAI_NULL_OBJECT_ID if given object id has invalid object type.
             */
            static otai_object_id_t linecardIdQuery(
                    _In_ otai_object_id_t objectId);

            /**
             * @brief Object type query.
             *
             * Returns object type for input object id. If object id is invalid
             * then returns OTAI_OBJECT_TYPE_NULL.
             */
            static otai_object_type_t objectTypeQuery(
                    _In_ otai_object_id_t objectId);

            /**
             * @brief Get object index.
             *
             * Returns object index.
             */
            static uint64_t getObjectIndex(
                    _In_ otai_object_id_t objectId);

        private:
            /**
             * @brief Oid index generator.
             */
            std::shared_ptr<OidIndexGenerator> m_oidIndexGenerator;
    };
}
