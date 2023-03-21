#pragma once

#include "LaiAttrWrapper.h"
#include "LaiObject.h"
#include "MetaKeyHasher.h"

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace laimeta
{
    class LaiObjectCollection
    {
        public:

            LaiObjectCollection() = default;
            virtual ~LaiObjectCollection() = default;

        private:

            LaiObjectCollection(const LaiObjectCollection&) = delete;
            LaiObjectCollection& operator=(const LaiObjectCollection&) = delete;

        public:

            void clear();

            bool objectExists(
                    _In_ const lai_object_meta_key_t& metaKey) const;

            void createObject(
                    _In_ const lai_object_meta_key_t& metaKey);

            void removeObject(
                    _In_ const lai_object_meta_key_t& metaKey);

            void setObjectAttr(
                    _In_ const lai_object_meta_key_t& metaKey,
                    _In_ const lai_attr_metadata_t& md,
                    _In_ const lai_attribute_t *attr);

            std::shared_ptr<LaiAttrWrapper> getObjectAttr(
                    _In_ const lai_object_meta_key_t& metaKey,
                    _In_ lai_attr_id_t id);

            std::vector<std::shared_ptr<LaiAttrWrapper>> getObjectAttributes(
                    _In_ const lai_object_meta_key_t& metaKey) const;

            std::vector<std::shared_ptr<LaiObject>> getObjectsByObjectType(
                    _In_ lai_object_type_t objectType);

            std::shared_ptr<LaiObject> getObject(
                    _In_ const lai_object_meta_key_t& metaKey) const;

            std::vector<lai_object_meta_key_t> getAllKeys() const;

        private:

            std::unordered_map<lai_object_meta_key_t, std::shared_ptr<LaiObject>, MetaKeyHasher, MetaKeyHasher> m_objects;
    };
}
