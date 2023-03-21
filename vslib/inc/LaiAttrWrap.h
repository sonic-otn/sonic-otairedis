#pragma once

extern "C" {
#include "laimetadata.h"
}

#include <string>

namespace laivs
{
    // TODO unify wrapper and add to common
    class LaiAttrWrap
    {
        private:

            LaiAttrWrap(const LaiAttrWrap&) = delete;
            LaiAttrWrap& operator=(const LaiAttrWrap&) = delete;

        public:

                LaiAttrWrap(
                        _In_ lai_object_type_t object_type,
                        _In_ const lai_attribute_t *attr);

                LaiAttrWrap(
                        _In_ const std::string& attrId,
                        _In_ const std::string& attrValue);

                virtual ~LaiAttrWrap();
                void updateValue();
        public:

                const lai_attribute_t* getAttr() const;

                const lai_attr_metadata_t* getAttrMetadata() const;

                const std::string& getAttrStrValue() const;

        private:

                const lai_attr_metadata_t *m_meta;

                lai_attribute_t m_attr;

                std::string m_value;
    };

}
