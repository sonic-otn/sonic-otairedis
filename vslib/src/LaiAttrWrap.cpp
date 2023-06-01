#include "LaiAttrWrap.h"

#include "swss/logger.h"
#include "meta/lai_serialize.h"

using namespace laivs;

LaiAttrWrap::LaiAttrWrap(
        _In_ lai_object_type_t object_type,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    m_meta = lai_metadata_get_attr_metadata(object_type, attr->id);

    m_attr.id = attr->id;

    /*
     * We are making serialize and deserialize to get copy of
     * attribute, it may be a list so we need to allocate new memory.
     *
     * This copy will be used later to get previous value of attribute
     * if attribute will be updated. And if this attribute is oid list
     * then we need to release object reference count.
     */

    m_value = lai_serialize_attr_value(*m_meta, *attr, false);

    lai_deserialize_attr_value(m_value, *m_meta, m_attr, false);
}

LaiAttrWrap::LaiAttrWrap(
        _In_ const std::string& attrId,
        _In_ const std::string& attrValue)
{
    SWSS_LOG_ENTER();

    m_meta = lai_metadata_get_attr_metadata_by_attr_id_name(attrId.c_str());

    if (m_meta == NULL)
    {
        SWSS_LOG_THROW("failed to find metadata for %s", attrId.c_str());
    }

    m_attr.id = m_meta->attrid;

    m_value = attrValue;

    lai_deserialize_attr_value(attrValue.c_str(), *m_meta, m_attr, false);
}

LaiAttrWrap::~LaiAttrWrap()
{
    SWSS_LOG_ENTER();

    /*
     * On destructor we need to call free to deallocate possible
     * allocated list on constructor.
     */

    lai_deserialize_free_attribute_value(m_meta->attrvaluetype, m_attr);
}

const lai_attribute_t* LaiAttrWrap::getAttr() const
{
    SWSS_LOG_ENTER();

    return &m_attr;
}

const lai_attr_metadata_t* LaiAttrWrap::getAttrMetadata() const
{
    SWSS_LOG_ENTER();

    return m_meta;
}

const std::string& LaiAttrWrap::getAttrStrValue() const
{
    SWSS_LOG_ENTER();

    return m_value;
}

void LaiAttrWrap::updateValue()
{
    switch (m_meta->attrvaluetype)
    {
        case LAI_ATTR_VALUE_TYPE_BOOL:
            m_attr.value.booldata = !m_attr.value.booldata;
            break;
        case LAI_ATTR_VALUE_TYPE_UINT8:
            m_attr.value.u8++;
            break;
        case LAI_ATTR_VALUE_TYPE_INT8:
            m_attr.value.s8++;
            break;
        case LAI_ATTR_VALUE_TYPE_UINT16:
            m_attr.value.u16++;
            break;
        case LAI_ATTR_VALUE_TYPE_INT16:
            m_attr.value.s16++;
            break;
        case LAI_ATTR_VALUE_TYPE_UINT32:
            m_attr.value.u32++;
            break;
        case LAI_ATTR_VALUE_TYPE_INT32:
            if (m_meta->isenum == false) {
                m_attr.value.s32++;
            }
            break;
        case LAI_ATTR_VALUE_TYPE_UINT64:
            m_attr.value.u64++;
            break;
        case LAI_ATTR_VALUE_TYPE_INT64:
            m_attr.value.s64++;
            break;
        case LAI_ATTR_VALUE_TYPE_DOUBLE:
            m_attr.value.d64 = m_attr.value.d64 + 1.0;
            break;
        default:
            break;
    }    
}
