#include "LaiObj.h"

#include "swss/logger.h"

using namespace syncd;

LaiObj::LaiObj():
    m_createdObject(false),
    m_objectStatus(LAI_OBJECT_STATUS_NOT_PROCESSED)
{
    SWSS_LOG_ENTER();

    // empty
}

bool LaiObj::isOidObject() const
{
    SWSS_LOG_ENTER();
    
    return m_info->isobjectid;
}

const std::unordered_map<lai_attr_id_t, std::shared_ptr<LaiAttr>>& LaiObj::getAllAttributes() const
{
    SWSS_LOG_ENTER();

    return m_attrs;
}

std::shared_ptr<const LaiAttr> LaiObj::getLaiAttr(
        _In_ lai_attr_id_t id) const
{
    SWSS_LOG_ENTER();

    auto it = m_attrs.find(id);

    if (it == m_attrs.end())
    {
        for (const auto &ita: m_attrs)
        {
            const auto &a = ita.second;

            SWSS_LOG_ERROR("%s: %s", a->getStrAttrId().c_str(), a->getStrAttrValue().c_str());
        }

        SWSS_LOG_THROW("object %s has no attribute %d", m_str_object_id.c_str(), id);
    }

    return it->second;
}

std::shared_ptr<const LaiAttr> LaiObj::tryGetLaiAttr(
        _In_ lai_attr_id_t id) const
{
    SWSS_LOG_ENTER();

    auto it = m_attrs.find(id);

    if (it == m_attrs.end())
        return nullptr;

    return it->second;
}

void LaiObj::setObjectStatus(
        _In_ lai_object_status_t objectStatus)
{
    SWSS_LOG_ENTER();

    m_objectStatus = objectStatus;
}

lai_object_status_t LaiObj::getObjectStatus() const
{
    SWSS_LOG_ENTER();

    return m_objectStatus;
}

lai_object_type_t LaiObj::getObjectType() const
{
    SWSS_LOG_ENTER();

    return m_meta_key.objecttype;
}

void LaiObj::setAttr(
        _In_ const std::shared_ptr<LaiAttr> &attr)
{
    SWSS_LOG_ENTER();

    m_attrs[attr->getLaiAttr()->id] = attr;
}

bool LaiObj::hasAttr(
        _In_ lai_attr_id_t id) const
{
    SWSS_LOG_ENTER();

    return m_attrs.find(id) != m_attrs.end();
}

lai_object_id_t LaiObj::getVid() const
{
    SWSS_LOG_ENTER();

    if (isOidObject())
    {
        return m_meta_key.objectkey.key.object_id;
    }

    SWSS_LOG_THROW("object %s it not object id type", m_str_object_id.c_str());
}
