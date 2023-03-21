#pragma once

extern "C" {
#include "laimetadata.h"
}

#include <string>
#include <vector>

namespace syncd
{
    /**
     * @brief Class represents single attribute
     *
     * Some attributes are lists, and have allocated memory, this class will help to
     * handle memory management and also will keep metadata for attribute.
     */
    class LaiAttr
    {
        private:

            /*
             * Copy constructor and assignment operator are marked as private to
             * prevent copy of this object, since attribute can contain pointers to
             * list which can lead to double free when object copy.
             *
             * This can be solved by making proper implementations of those
             * methods, currently this is not required.
             */

            LaiAttr(const LaiAttr&) = delete;
            LaiAttr& operator=(const LaiAttr&) = delete;

        public:

            /**
             * @brief Constructor
             *
             * @param[in] str_attr_id Attribute is as string
             * @param[in] str_attr_value Attribute value as string
             */
            LaiAttr(
                    _In_ const std::string &str_attr_id,
                    _In_ const std::string &str_attr_value);

            virtual ~LaiAttr();

        public:

            lai_attribute_t* getRWLaiAttr();

            const lai_attribute_t* getLaiAttr() const;

            /**
             * @brief Gets value.oid of attribute value.
             *
             * If attribute is not OID exception will be thrown.
             *
             * @return Oid field of attribute value.
             */
            lai_object_id_t getOid() const;

            /**
             * @brief Tells whether attribute contains OIDs
             *
             * @return True if attribute contains OIDs, false otherwise
             */
            bool isObjectIdAttr() const;

                const std::string& getStrAttrId() const;

            const std::string& getStrAttrValue() const;

            const lai_attr_metadata_t* getAttrMetadata() const;

            void updateValue();

            /**
             * @brief Get OID list from attribute
             *
             * Based on serialization type attribute may be oid attribute, oid list
             * attribute or non oid attribute. This method will extract all those oids from
             * this attribute and return as vector.  This is handy when we need processing
             * oids per attribute.
             *
             * @return Object list used in attribute
             */
            std::vector<lai_object_id_t> getOidListFromAttribute() const;

        private:

            const std::string m_str_attr_id;

            std::string m_str_attr_value;

            const lai_attr_metadata_t* m_meta;

            lai_attribute_t m_attr;
    };
}
