#pragma once

extern "C" {
#include "otaimetadata.h"
}

#include <string>

namespace otaivs
{
    // TODO unify wrapper and add to common
    class OtaiAttrWrap
    {
        private:

            OtaiAttrWrap(const OtaiAttrWrap&) = delete;
            OtaiAttrWrap& operator=(const OtaiAttrWrap&) = delete;

        public:

                OtaiAttrWrap(
                        _In_ otai_object_type_t object_type,
                        _In_ const otai_attribute_t *attr);

                OtaiAttrWrap(
                        _In_ const std::string& attrId,
                        _In_ const std::string& attrValue);

                virtual ~OtaiAttrWrap();
                void updateValue();
        public:

                const otai_attribute_t* getAttr() const;

                const otai_attr_metadata_t* getAttrMetadata() const;

                const std::string& getAttrStrValue() const;

        private:

                const otai_attr_metadata_t *m_meta;

                otai_attribute_t m_attr;

                std::string m_value;
    };

}
