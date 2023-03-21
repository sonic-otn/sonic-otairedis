#include "LaiAttributeList.h"

#include "lai_serialize.h"

using namespace laimeta;

LaiAttributeList::LaiAttributeList(
        _In_ const lai_object_type_t objectType,
        _In_ const std::vector<swss::FieldValueTuple> &values,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    size_t attr_count = values.size();

    for (size_t i = 0; i < attr_count; ++i)
    {
        const std::string &str_attr_id = fvField(values[i]);
        const std::string &str_attr_value = fvValue(values[i]);

        if (str_attr_id == "NULL")
        {
            continue;
        }

        lai_attribute_t attr;
        memset(&attr, 0, sizeof(lai_attribute_t));

        lai_deserialize_attr_id(str_attr_id, attr.id);

        auto meta = lai_metadata_get_attr_metadata(objectType, attr.id);

        if (meta == NULL)
        {
            SWSS_LOG_THROW("FATAL: failed to find metadata for object type %d and attr id %d", objectType, attr.id);
        }

        lai_deserialize_attr_value(str_attr_value, *meta, attr, countOnly);

        m_attr_list.push_back(attr);
        m_attr_value_type_list.push_back(meta->attrvaluetype);
    }
}

LaiAttributeList::LaiAttributeList(
        _In_ const lai_object_type_t objectType,
        _In_ const std::unordered_map<std::string, std::string>& hash,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    for (auto it = hash.begin(); it != hash.end(); it++)
    {
        const std::string &str_attr_id = it->first;
        const std::string &str_attr_value = it->second;

        if (str_attr_id == "NULL")
        {
            continue;
        }

        lai_attribute_t attr;
        memset(&attr, 0, sizeof(lai_attribute_t));

        lai_deserialize_attr_id(str_attr_id, attr.id);

        auto meta = lai_metadata_get_attr_metadata(objectType, attr.id);

        if (meta == NULL)
        {
            SWSS_LOG_THROW("FATAL: failed to find metadata for object type %d and attr id %d", objectType, attr.id);
        }

        lai_deserialize_attr_value(str_attr_value, *meta, attr, countOnly);

        m_attr_list.push_back(attr);
        m_attr_value_type_list.push_back(meta->attrvaluetype);
    }
}

LaiAttributeList::~LaiAttributeList()
{
    SWSS_LOG_ENTER();

    size_t attr_count = m_attr_list.size();

    for (size_t i = 0; i < attr_count; ++i)
    {
        lai_attribute_t &attr = m_attr_list[i];

        lai_attr_value_type_t serialization_type = m_attr_value_type_list[i];

        lai_deserialize_free_attribute_value(serialization_type, attr);
    }
}

std::vector<swss::FieldValueTuple> LaiAttributeList::serialize_attr_list(
        _In_ lai_object_type_t objectType,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    std::vector<swss::FieldValueTuple> entry;

    for (uint32_t index = 0; index < attr_count; ++index)
    {
        const lai_attribute_t *attr = &attr_list[index];

        auto meta = lai_metadata_get_attr_metadata(objectType, attr->id);

        if (meta == NULL)
        {
            SWSS_LOG_THROW("FATAL: failed to find metadata for object type %d and attr id %d", objectType, attr->id);
        }

        std::string str_attr_id = lai_serialize_attr_id(*meta);

        std::string str_attr_value = lai_serialize_attr_value(*meta, *attr, countOnly);

        swss::FieldValueTuple fvt(str_attr_id, str_attr_value);

        entry.push_back(fvt);
    }

    return entry;
}

lai_attribute_t* LaiAttributeList::get_attr_list()
{
    SWSS_LOG_ENTER();

    return m_attr_list.data();
}

uint32_t LaiAttributeList::get_attr_count()
{
    SWSS_LOG_ENTER();

    return (uint32_t)m_attr_list.size();
}
