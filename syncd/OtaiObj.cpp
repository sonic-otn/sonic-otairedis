#include "OtaiObj.h"

#include "swss/logger.h"

using namespace syncd;

OtaiObj::OtaiObj():
    m_createdObject(false),
    m_objectStatus(OTAI_OBJECT_STATUS_NOT_PROCESSED)
{
    SWSS_LOG_ENTER();

    // empty
}

bool OtaiObj::isOidObject() const
{
    SWSS_LOG_ENTER();
    
    return m_info->isobjectid;
}

const std::unordered_map<otai_attr_id_t, std::shared_ptr<OtaiAttr>>& OtaiObj::getAllAttributes() const
{
    SWSS_LOG_ENTER();

    return m_attrs;
}

std::shared_ptr<const OtaiAttr> OtaiObj::getOtaiAttr(
        _In_ otai_attr_id_t id) const
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

std::shared_ptr<const OtaiAttr> OtaiObj::tryGetOtaiAttr(
        _In_ otai_attr_id_t id) const
{
    SWSS_LOG_ENTER();

    auto it = m_attrs.find(id);

    if (it == m_attrs.end())
        return nullptr;

    return it->second;
}

void OtaiObj::setObjectStatus(
        _In_ otai_object_status_t objectStatus)
{
    SWSS_LOG_ENTER();

    m_objectStatus = objectStatus;
}

otai_object_status_t OtaiObj::getObjectStatus() const
{
    SWSS_LOG_ENTER();

    return m_objectStatus;
}

otai_object_type_t OtaiObj::getObjectType() const
{
    SWSS_LOG_ENTER();

    return m_meta_key.objecttype;
}

void OtaiObj::setAttr(
        _In_ const std::shared_ptr<OtaiAttr> &attr)
{
    SWSS_LOG_ENTER();

    m_attrs[attr->getOtaiAttr()->id] = attr;
}

bool OtaiObj::hasAttr(
        _In_ otai_attr_id_t id) const
{
    SWSS_LOG_ENTER();

    return m_attrs.find(id) != m_attrs.end();
}

otai_object_id_t OtaiObj::getVid() const
{
    SWSS_LOG_ENTER();

    if (isOidObject())
    {
        return m_meta_key.objectkey.key.object_id;
    }

    SWSS_LOG_THROW("object %s it not object id type", m_str_object_id.c_str());
}
