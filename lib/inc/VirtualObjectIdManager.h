#pragma once

extern "C" {
#include "otai.h"
}

#include "OidIndexGenerator.h"
#include "LinecardConfigContainer.h"

#include <set>
#include <memory>

namespace otairedis
{
    class VirtualObjectIdManager
    {

        public:

            VirtualObjectIdManager(
                    _In_ uint32_t globalContext,
                    _In_ std::shared_ptr<LinecardConfigContainer> scc,
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
            otai_object_id_t allocateNewLinecardObjectId(
                    _In_ const std::string& hardwareInfo);

            /**
             * @brief release allocated object id.
             *
             * If object type is linecard, then linecard index will be released.
             */
            void releaseObjectId(
                    _In_ otai_object_id_t objectId);

        private:

            /**
             * @brief Release given linecard index.
             *
             * Will throw if index is not allocated.
             */
            void releaseLinecardIndex(
                    _In_ uint32_t index);

            /**
             * @brief Allocate new linecard index.
             *
             * Will throw if there are no more available linecard indexes.
             */
            uint32_t allocateNewLinecardIndex();

            /**
             * @brief Construct object id.
             *
             * Using all input parameters to construct object id.
             */
            static otai_object_id_t constructObjectId(
                    _In_ otai_object_type_t objectType,
                    _In_ uint32_t linecardIndex,
                    _In_ uint64_t objectIndex,
                    _In_ uint32_t globalContext);

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
             * @brief Get linecard index.
             *
             * Index range is <0..255>.
             *
             * Returns linecard index for given oid. If oid is invalid, returns 0.
             */
            static uint32_t getLinecardIndex(
                    _In_ otai_object_id_t objectId);

            /**
             * @brief Get global context.
             *
             * Index range is <0..255>.
             *
             * Returns global context for given oid. If oid is invalid, returns 0.
             */
            static uint32_t getGlobalContext(
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

        private:

            /**
             * @brief Global context value.
             *
             * Will be encoded in every object id, and it will point to global
             * (system wide) syncd instance.
             */
            uint32_t m_globalContext;

            std::shared_ptr<LinecardConfigContainer> m_container;

            /**
             * @brief Oid index generator.
             */
            std::shared_ptr<OidIndexGenerator> m_oidIndexGenerator;

            /**
             * @brief Set of allocated linecard indexes.
             */
            std::set<uint32_t> m_linecardIndexes;
    };
}
