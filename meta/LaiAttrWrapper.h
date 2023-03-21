#pragma once

extern "C" {
#include "laimetadata.h"
}

#include "swss/sal.h"

namespace laimeta
{
    class LaiAttrWrapper
    {
        public:

            LaiAttrWrapper(
                    _In_ const lai_attr_metadata_t* meta,
                    _In_ const lai_attribute_t& attr);

            virtual ~LaiAttrWrapper();

        public:

            const lai_attribute_t* getLaiAttr() const;

            const lai_attr_metadata_t* getLaiAttrMetadata() const;

            lai_attr_id_t getAttrId() const;

        private:

            LaiAttrWrapper(const LaiAttrWrapper&) = delete;
            LaiAttrWrapper& operator=(const LaiAttrWrapper&) = delete;

        private:

            /**
             * @brief Attribute metadata associated with given attribute.
             */
            const lai_attr_metadata_t* m_meta;

            lai_attribute_t m_attr;
    };
}
