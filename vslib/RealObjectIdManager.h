#pragma once

extern "C" {
#include <otai.h>
}

#include <set>
#include <map>

namespace otaivs
{
    static std::map<otai_object_type_t, uint64_t> m_indexer;
    
    class RealObjectIdManager
    {
        public:
            /**
             * @brief Allocate new object id on a given linecard.
             *
             * Method can't be used to allocate linecard object id.
             *
             * Throws when object type is linecard and there are no more
             * available linecard indexes.
             */
            static otai_object_id_t allocateNewObjectId(
                    _In_ otai_object_type_t objectType,
                    _In_ otai_object_id_t linecardId,
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);

            /**
             * @brief Allocate new linecard object id.
             */
            static otai_object_id_t allocateNewLinecardObjectId();

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

            static uint32_t getObjectIndex(
                    _In_ otai_object_id_t objectId);
        private:
            /**
             * @brief Construct object id.
             *
             * Using all input parameters to construct object id.
             */
            static otai_object_id_t constructObjectId(
                    _In_ otai_object_type_t objectType,
                    _In_ uint32_t linecardIndex,
                    _In_ uint64_t objectIndex,
                    _In_ uint32_t globalContext = 0);

            static uint64_t allocateNewObjectIndex(
                    _In_ otai_object_type_t objectType,
                    _In_ uint32_t attr_count,
                    _In_ const otai_attribute_t *attr_list);
    };
}
