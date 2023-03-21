#include "LaiAttrWrapper.h"

#include "swss/logger.h"

#include "lai_serialize.h"

using namespace laimeta;

LaiAttrWrapper::LaiAttrWrapper(
        _In_ const lai_attr_metadata_t* meta,
        _In_ const lai_attribute_t& attr):
    m_meta(meta),
    m_attr(attr)
{
    SWSS_LOG_ENTER();

    if (!meta)
    {
        SWSS_LOG_THROW("metadata can't be null");
    }

    m_attr.id = attr.id;

    /*
     * We are making serialize and deserialize to get copy of attribute, it may
     * be a list so we need to allocate new memory.
     *
     * This copy will be used later to get previous value of attribute if
     * attribute will be updated. And if this attribute is oid list then we
     * need to release object reference count.
     */

    std::string str = lai_serialize_attr_value(*meta, attr, false);

    lai_deserialize_attr_value(str, *meta, m_attr, false);
}

LaiAttrWrapper::~LaiAttrWrapper()
{
    SWSS_LOG_ENTER();

    /*
     * On destructor we need to call free to deallocate possible allocated
     * memory on attribute value.
     */

    lai_deserialize_free_attribute_value(m_meta->attrvaluetype, m_attr);
}

const lai_attribute_t* LaiAttrWrapper::getLaiAttr() const
{
    SWSS_LOG_ENTER();

    return &m_attr;
}

const lai_attr_metadata_t* LaiAttrWrapper::getLaiAttrMetadata() const
{
    SWSS_LOG_ENTER();

    return m_meta;
}

lai_attr_id_t LaiAttrWrapper::getAttrId() const
{
    SWSS_LOG_ENTER();

    return m_attr.id;
}

