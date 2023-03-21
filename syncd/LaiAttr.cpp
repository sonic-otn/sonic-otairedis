#include "LaiAttr.h"

#include "swss/logger.h"
#include "meta/lai_serialize.h"

using namespace syncd;

LaiAttr::LaiAttr(
        _In_ const std::string &str_attr_id,
        _In_ const std::string &str_attr_value):
    m_str_attr_id(str_attr_id),
    m_str_attr_value(str_attr_value),
    m_meta(NULL)
{
    SWSS_LOG_ENTER();

    /*
     * We perform deserialize here to have attribute value when we need
     * it, this can include allocated lists, so on destructor we need
     * to free this memory.
     */

    lai_deserialize_attr_id(str_attr_id, &m_meta);

    m_attr.id = m_meta->attrid;

    lai_deserialize_attr_value(str_attr_value, *m_meta, m_attr, false);

    if (m_meta->isenum && m_meta->enummetadata->ignorevalues)
    {
        // if attribute is enum, and we are loading some older LAI values it
        // may happen that we get deprecated/ignored value string and we need
        // to update it to current one to not cause attribute compare confusion
        // since they are compared by string value

        auto val = lai_serialize_enum(m_attr.value.s32, m_meta->enummetadata);

        if (val != m_str_attr_value)
        {
            SWSS_LOG_NOTICE("translating deprecated/ignore enum %s to %s",
                    m_str_attr_value.c_str(),
                    val.c_str());

            m_str_attr_value = val;
        }
    }
}

LaiAttr::~LaiAttr()
{
    SWSS_LOG_ENTER();

    lai_deserialize_free_attribute_value(m_meta->attrvaluetype, m_attr);
}

lai_attribute_t* LaiAttr::getRWLaiAttr()
{
    SWSS_LOG_ENTER();

    return &m_attr;
}

const lai_attribute_t* LaiAttr::getLaiAttr() const
{
    SWSS_LOG_ENTER();

    return &m_attr;
}

lai_object_id_t LaiAttr::getOid() const
{
    SWSS_LOG_ENTER();

    if (m_meta->attrvaluetype == LAI_ATTR_VALUE_TYPE_OBJECT_ID)
    {
        return m_attr.value.oid;
    }

    SWSS_LOG_THROW("attribute %s is not OID attribute", m_meta->attridname);
}

bool LaiAttr::isObjectIdAttr() const
{
    SWSS_LOG_ENTER();

    return m_meta->isoidattribute;
}

const std::string& LaiAttr::getStrAttrId() const
{
    SWSS_LOG_ENTER();

    return m_str_attr_id;
}

const std::string& LaiAttr::getStrAttrValue() const
{
    SWSS_LOG_ENTER();

    return m_str_attr_value;
}

const lai_attr_metadata_t* LaiAttr::getAttrMetadata() const
{
    SWSS_LOG_ENTER();

    return m_meta;
}

void LaiAttr::updateValue()
{
    SWSS_LOG_ENTER();

    m_str_attr_value = lai_serialize_attr_value(*m_meta, m_attr);
}

std::vector<lai_object_id_t> LaiAttr::getOidListFromAttribute() const
{
    SWSS_LOG_ENTER();

    const lai_attribute_t &attr = m_attr;

    uint32_t count = 0;

    const lai_object_id_t *objectIdList = NULL;

    /*
     * For ACL fields and actions we need to use enable flag as
     * indicator, since when attribute is disabled then parameter can
     * be garbage.
     */

    switch (m_meta->attrvaluetype)
    {
        case LAI_ATTR_VALUE_TYPE_OBJECT_ID:

            count = 1;
            objectIdList = &attr.value.oid;

            break;

        case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:

            count = attr.value.objlist.count;
            objectIdList = attr.value.objlist.list;

            break;

        default:

            if (m_meta->isoidattribute)
            {
                SWSS_LOG_THROW("attribute %s is oid attribute but not handled", m_meta->attridname);
            }

            // Attribute not contain any object ids.

            break;
    }

    std::vector<lai_object_id_t> result(objectIdList, objectIdList + count);

    return result;
}
