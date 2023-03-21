#pragma once

extern "C" {
#include "laimetadata.h"
}

#include "swss/table.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace laimeta
{
    class LaiAttributeList
    {
        public:

            LaiAttributeList(
                    _In_ const lai_object_type_t object_type,
                    _In_ const std::vector<swss::FieldValueTuple> &values,
                    _In_ bool countOnly);

            LaiAttributeList(
                    _In_ const lai_object_type_t object_type,
                    _In_ const std::unordered_map<std::string, std::string>& hash,
                    _In_ bool countOnly);

            virtual ~LaiAttributeList();

        public:

            lai_attribute_t* get_attr_list();

            uint32_t get_attr_count();

            static std::vector<swss::FieldValueTuple> serialize_attr_list(
                    _In_ lai_object_type_t object_type,
                    _In_ uint32_t attr_count,
                    _In_ const lai_attribute_t *attr_list,
                    _In_ bool countOnly);

        private:

            LaiAttributeList(const LaiAttributeList&);
            LaiAttributeList& operator=(const LaiAttributeList&);

            std::vector<lai_attribute_t> m_attr_list;
            std::vector<lai_attr_value_type_t> m_attr_value_type_list;
    };
}
