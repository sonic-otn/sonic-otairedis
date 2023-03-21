#include "LaiObject.h"

#include "swss/logger.h"

#include "lai_serialize.h"

using namespace laimeta;

LaiObject::LaiObject(
        _In_ const lai_object_meta_key_t& metaKey):
    m_metaKey(metaKey)
{
    SWSS_LOG_ENTER();

    // empty
}

lai_object_type_t LaiObject::getObjectType() const
{
    SWSS_LOG_ENTER();

    return m_metaKey.objecttype;
}

bool LaiObject::hasAttr(
        _In_ lai_attr_id_t id) const
{
    SWSS_LOG_ENTER();

    return m_attrs.find(id) != m_attrs.end();
}

const lai_object_meta_key_t& LaiObject::getMetaKey() const
{
    SWSS_LOG_ENTER();

    return m_metaKey;
}

void LaiObject::setAttr(
        _In_ const lai_attr_metadata_t* md,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    m_attrs[attr->id] = std::make_shared<LaiAttrWrapper>(md, *attr);
}

void LaiObject::setAttr(
        _In_ std::shared_ptr<LaiAttrWrapper> attr)
{
    SWSS_LOG_ENTER();

    m_attrs[attr->getAttrId()] = attr;
}

std::shared_ptr<LaiAttrWrapper> LaiObject::getAttr(
        _In_ lai_attr_id_t id) const
{
    SWSS_LOG_ENTER();

    auto it = m_attrs.find(id);

    if (it != m_attrs.end())
        return it->second;

    return nullptr;
}

std::vector<std::shared_ptr<LaiAttrWrapper>> LaiObject::getAttributes() const
{
    SWSS_LOG_ENTER();

    std::vector<std::shared_ptr<LaiAttrWrapper>> values;

    for (auto&kvp: m_attrs)
        values.push_back(kvp.second);

    return values;
}

