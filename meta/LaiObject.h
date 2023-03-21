#pragma once

#include "LaiAttrWrapper.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace laimeta
{
    class LaiObject
    {
        public:

            LaiObject(
                    _In_ const lai_object_meta_key_t& metaKey);

            virtual ~LaiObject() = default;

        private:

            LaiObject(const LaiObject&) = delete;
            LaiObject& operator=(const LaiObject&) = delete;

        public:

            lai_object_type_t getObjectType() const;

            bool hasAttr(
                    _In_ lai_attr_id_t id) const;

            const lai_object_meta_key_t& getMetaKey() const;

            void setAttr(
                    _In_ const lai_attr_metadata_t* md,
                    _In_ const lai_attribute_t *attr);

            void setAttr(
                    _In_ std::shared_ptr<LaiAttrWrapper> attr);

            std::shared_ptr<LaiAttrWrapper> getAttr(
                    _In_ lai_attr_id_t id) const;

            std::vector<std::shared_ptr<LaiAttrWrapper>> getAttributes() const;

        private:

            lai_object_meta_key_t m_metaKey;

            std::unordered_map<lai_attr_id_t, std::shared_ptr<LaiAttrWrapper>> m_attrs;
    };
}
