#include "LaiObjectCollection.h"

#include "lai_serialize.h"

using namespace laimeta;

void LaiObjectCollection::clear()
{
    SWSS_LOG_ENTER();

    m_objects.clear();
}

bool LaiObjectCollection::objectExists(
        _In_ const lai_object_meta_key_t& metaKey) const
{
    SWSS_LOG_ENTER();

    bool exists = m_objects.find(metaKey) != m_objects.end();

    return exists;
}

void LaiObjectCollection::createObject(
        _In_ const lai_object_meta_key_t& metaKey)
{
    SWSS_LOG_ENTER();

    auto obj = std::make_shared<LaiObject>(metaKey);

    if (objectExists(metaKey))
    {
        SWSS_LOG_THROW("FATAL: object %s already exists",
                lai_serialize_object_meta_key(metaKey).c_str());
    }

    m_objects[metaKey] = obj;
}

void LaiObjectCollection::removeObject(
        _In_ const lai_object_meta_key_t& metaKey)
{
    SWSS_LOG_ENTER();

    if (!objectExists(metaKey))
    {
        SWSS_LOG_THROW("FATAL: object %s doesn't exist",
                lai_serialize_object_meta_key(metaKey).c_str());
    }

    m_objects.erase(metaKey);
}

void LaiObjectCollection::setObjectAttr(
        _In_ const lai_object_meta_key_t& metaKey,
        _In_ const lai_attr_metadata_t& md,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    if (!objectExists(metaKey))
    {
        SWSS_LOG_THROW("FATAL: object %s doesn't exist",
               lai_serialize_object_meta_key(metaKey).c_str());
    }

    m_objects[metaKey]->setAttr(&md, attr);
}

std::shared_ptr<LaiAttrWrapper> LaiObjectCollection::getObjectAttr(
        _In_ const lai_object_meta_key_t& metaKey,
        _In_ lai_attr_id_t id)
{
    SWSS_LOG_ENTER();

    /*
     * We can't throw if object don't exists, since we can call this function
     * on create API, and then previous object will not exists, of course we
     * should make exists check before.
     */

    auto it = m_objects.find(metaKey);

    if (it == m_objects.end())
    {
        SWSS_LOG_ERROR("object key %s not found",
                lai_serialize_object_meta_key(metaKey).c_str());

        return nullptr;
    }

    return it->second->getAttr(id);
}

std::vector<std::shared_ptr<LaiObject>> LaiObjectCollection::getObjectsByObjectType(
        _In_ lai_object_type_t objectType)
{
    SWSS_LOG_ENTER();

    std::vector<std::shared_ptr<LaiObject>> vec;

    for (auto& kvp: m_objects)
    {
        if (kvp.second->getObjectType() == objectType)
            vec.push_back(kvp.second);
    }

    return vec;
}

std::shared_ptr<LaiObject> LaiObjectCollection::getObject(
        _In_ const lai_object_meta_key_t& metaKey) const
{
    SWSS_LOG_ENTER();

    if (!objectExists(metaKey))
    {
        SWSS_LOG_THROW("FATAL: object %s doesn't exist",
                lai_serialize_object_meta_key(metaKey).c_str());
    }

    return m_objects.at(metaKey);
}

std::vector<lai_object_meta_key_t> LaiObjectCollection::getAllKeys() const
{
    SWSS_LOG_ENTER();

    std::vector<lai_object_meta_key_t> vec;

    for (auto& it: m_objects)
    {
        vec.push_back(it.first);
    }

    return vec;
}
