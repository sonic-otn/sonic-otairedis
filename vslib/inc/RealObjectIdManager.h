#pragma once

extern "C" {
#include <lai.h>
}

#include "LinecardConfigContainer.h"

#include <set>
#include <map>

namespace laivs
{
    class RealObjectIdManager
    {
        public:

            RealObjectIdManager(
                    _In_ uint32_t globalContext,
                    _In_ std::shared_ptr<LinecardConfigContainer> container);

            virtual ~RealObjectIdManager() = default;

        public:

            /**
             * @brief Linecard id query.
             *
             * Return linecard object id for given object if. If object type is
             * linecard, it will return input value.
             *
             * Throws if given object id has invalid object type.
             *
             * For LAI_NULL_OBJECT_ID input will return LAI_NULL_OBJECT_ID.
             */
            lai_object_id_t laiLinecardIdQuery(
                    _In_ lai_object_id_t objectId) const;

            /**
             * @brief Object type query.
             *
             * Returns object type for input object id. If object id is invalid
             * then returns LAI_OBJECT_TYPE_NULL.
             */
            lai_object_type_t laiObjectTypeQuery(
                    _In_ lai_object_id_t objectId) const;

            /**
             * @brief Clear linecard index set.
             *
             * New linecard index allocation will start from the beginning.
             */
            void clear();

            uint64_t allocateNewObjectIndex(
                    _In_ lai_object_type_t objectType,
                    _In_ uint32_t attr_count,
                    _In_ const lai_attribute_t *attr_list);

            /**
             * @brief Allocate new object id on a given linecard.
             *
             * Method can't be used to allocate linecard object id.
             *
             * Throws when object type is linecard and there are no more
             * available linecard indexes.
             */
            lai_object_id_t allocateNewObjectId(
                    _In_ lai_object_type_t objectType,
                    _In_ lai_object_id_t linecardId,
                    _In_ uint32_t attr_count,
                    _In_ const lai_attribute_t *attr_list);

            /**
             * @brief Allocate new linecard object id.
             */
            lai_object_id_t allocateNewLinecardObjectId(
                    _In_ const std::string& hardwareInfo);

            /**
             * @brief Release allocated object id.
             *
             * If object type is linecard, then linecard index will be released.
             */
            void releaseObjectId(
                    _In_ lai_object_id_t objectId);

        public:

            /**
             * @brief Linecard id query.
             *
             * Return linecard object id for given object if. If object type is
             * linecard, it will return input value.
             *
             * Return LAI_NULL_OBJECT_ID if given object id has invalid object type.
             */
            static lai_object_id_t linecardIdQuery(
                    _In_ lai_object_id_t objectId);

            /**
             * @brief Object type query.
             *
             * Returns object type for input object id. If object id is invalid
             * then returns LAI_OBJECT_TYPE_NULL.
             */
            static lai_object_type_t objectTypeQuery(
                    _In_ lai_object_id_t objectId);

            /**
             * @brief Get linecard index.
             *
             * Index range is <0..255>.
             *
             * Returns linecard index for given oid. If oid is invalid, returns 0.
             */
            static uint32_t getLinecardIndex(
                    _In_ lai_object_id_t objectId);

            uint32_t getObjectIndex(
                    _In_ lai_object_id_t objectId);
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
            static lai_object_id_t constructObjectId(
                    _In_ lai_object_type_t objectType,
                    _In_ uint32_t linecardIndex,
                    _In_ uint64_t objectIndex,
                    _In_ uint32_t globalContext);

        private:

            /**
             * @brief Global context value.
             *
             * Will be encoded in every object id, and it will point to global
             * (system wide) syncd instance.
             */
            uint32_t m_globalContext;

            /**
             * @brief Set of allocated linecard indexes.
             */
            std::set<uint32_t> m_linecardIndexes;

            std::map<lai_object_type_t, uint64_t> m_indexer;

            std::shared_ptr<LinecardConfigContainer> m_container;
    };
}
