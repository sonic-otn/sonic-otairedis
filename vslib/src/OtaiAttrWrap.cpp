#include "OtaiAttrWrap.h"

#include "swss/logger.h"
#include "meta/otai_serialize.h"

using namespace otaivs;

OtaiAttrWrap::OtaiAttrWrap(
        _In_ otai_object_type_t object_type,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    m_meta = otai_metadata_get_attr_metadata(object_type, attr->id);

    m_attr.id = attr->id;

    /*
     * We are making serialize and deserialize to get copy of
     * attribute, it may be a list so we need to allocate new memory.
     *
     * This copy will be used later to get previous value of attribute
     * if attribute will be updated. And if this attribute is oid list
     * then we need to release object reference count.
     */

    m_value = otai_serialize_attr_value(*m_meta, *attr, false);

    otai_deserialize_attr_value(m_value, *m_meta, m_attr, false);
}

OtaiAttrWrap::OtaiAttrWrap(
        _In_ const std::string& attrId,
        _In_ const std::string& attrValue)
{
    SWSS_LOG_ENTER();

    m_meta = otai_metadata_get_attr_metadata_by_attr_id_name(attrId.c_str());

    if (m_meta == NULL)
    {
        SWSS_LOG_THROW("failed to find metadata for %s", attrId.c_str());
    }

    m_attr.id = m_meta->attrid;

    m_value = attrValue;

    otai_deserialize_attr_value(attrValue.c_str(), *m_meta, m_attr, false);
}

OtaiAttrWrap::~OtaiAttrWrap()
{
    SWSS_LOG_ENTER();

    /*
     * On destructor we need to call free to deallocate possible
     * allocated list on constructor.
     */

    otai_deserialize_free_attribute_value(m_meta->attrvaluetype, m_attr);
}

const otai_attribute_t* OtaiAttrWrap::getAttr() const
{
    SWSS_LOG_ENTER();

    return &m_attr;
}

const otai_attr_metadata_t* OtaiAttrWrap::getAttrMetadata() const
{
    SWSS_LOG_ENTER();

    return m_meta;
}

const std::string& OtaiAttrWrap::getAttrStrValue() const
{
    SWSS_LOG_ENTER();

    return m_value;
}

void OtaiAttrWrap::updateValue()
{
    switch (m_meta->attrvaluetype)
    {
        case OTAI_ATTR_VALUE_TYPE_BOOL:
            m_attr.value.booldata = !m_attr.value.booldata;
            break;
        case OTAI_ATTR_VALUE_TYPE_UINT8:
            m_attr.value.u8++;
            break;
        case OTAI_ATTR_VALUE_TYPE_INT8:
            m_attr.value.s8++;
            break;
        case OTAI_ATTR_VALUE_TYPE_UINT16:
            m_attr.value.u16++;
            break;
        case OTAI_ATTR_VALUE_TYPE_INT16:
            m_attr.value.s16++;
            break;
        case OTAI_ATTR_VALUE_TYPE_UINT32:
            m_attr.value.u32++;
            break;
        case OTAI_ATTR_VALUE_TYPE_INT32:
            if (m_meta->isenum == false) {
                m_attr.value.s32++;
            }
            break;
        case OTAI_ATTR_VALUE_TYPE_UINT64:
            m_attr.value.u64++;
            break;
        case OTAI_ATTR_VALUE_TYPE_INT64:
            m_attr.value.s64++;
            break;
        case OTAI_ATTR_VALUE_TYPE_DOUBLE:
            m_attr.value.d64 = m_attr.value.d64 + 1.0;
            break;
        default:
            break;
    }    
}
