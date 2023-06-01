#include "Meta.h"

#include "swss/logger.h"
#include "lai_serialize.h"

#include "Globals.h"
#include "LaiAttributeList.h"

#include <inttypes.h>

#include <set>

// TODO add validation for all oids belong to the same linecard

#define MAX_LIST_COUNT 0x1000

#define CHECK_STATUS_SUCCESS(s) { if ((s) != LAI_STATUS_SUCCESS) return (s); }

#define VALIDATION_LIST(md,vlist) \
{\
    auto status1 = meta_genetic_validation_list(md,vlist.count,vlist.list);\
    if (status1 != LAI_STATUS_SUCCESS)\
    {\
        return status1;\
    }\
}

#define VALIDATION_LIST_GET(md, list) \
{\
    if (list.count > MAX_LIST_COUNT)\
    {\
        META_LOG_ERROR(md, "list count %u > max list count %u", list.count, MAX_LIST_COUNT);\
    }\
}

using namespace laimeta;

Meta::Meta(
        _In_ std::shared_ptr<lairedis::LaiInterface> impl):
    m_implementation(impl)
{
    SWSS_LOG_ENTER();

    m_unittestsEnabled = false;

    // TODO if metadata supports multiple linecards
    // then warm boot must be per each linecard

    m_warmBoot = false;
}

lai_status_t Meta::initialize(
        _In_ uint64_t flags,
        _In_ const lai_service_method_table_t *service_method_table)
{
    SWSS_LOG_ENTER();

    return m_implementation->initialize(flags, service_method_table);
}

lai_status_t Meta::uninitialize(void)
{
    SWSS_LOG_ENTER();

    return m_implementation->uninitialize();
}

lai_status_t Meta::linkCheck(_Out_ bool *up)
{
    SWSS_LOG_ENTER();

    return m_implementation->linkCheck(up);
}

void Meta::meta_init_db()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("begin");

    /*
     * This DB will contain objects from all linecards.
     *
     * TODO: later on we will have separate bases for each linecard.  This way
     * should be easier to manage, on remove linecard we will just clear that db,
     * instead of checking all objects.
     */

    m_oids.clear();
    m_laiObjectCollection.clear();
    m_attrKeys.clear();
    m_portRelatedSet.clear();

    // m_meta_unittests_set_readonly_set.clear();
    // m_unittestsEnabled = false

    m_warmBoot = false;

    SWSS_LOG_NOTICE("end");
}

bool Meta::isEmpty()
{
    SWSS_LOG_ENTER();

    return m_portRelatedSet.getAllPorts().empty()
        && m_oids.getAllOids().empty()
        && m_attrKeys.getAllKeys().empty()
        && m_laiObjectCollection.getAllKeys().empty();
}

lai_status_t Meta::remove(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id)
{
    SWSS_LOG_ENTER();

    lai_status_t status = meta_lai_validate_oid(object_type, &object_id, LAI_NULL_OBJECT_ID, false);

    if (status != LAI_STATUS_SUCCESS)
    {
        return status;
    }

    lai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = object_id } } };

    status = meta_generic_validation_remove(meta_key);

    if (status != LAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = m_implementation->remove(object_type, object_id);

    if (status == LAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("remove status: %s", lai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("remove status: %s", lai_serialize_status(status).c_str());
    }

    if (status == LAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_remove(meta_key);
    }

    return status;
}

lai_status_t Meta::create(
        _In_ lai_object_type_t object_type,
        _Out_ lai_object_id_t* object_id,
        _In_ lai_object_id_t linecard_id,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    lai_status_t status = meta_lai_validate_oid(object_type, object_id, linecard_id, true);

    if (status != LAI_STATUS_SUCCESS)
    {
        return status;
    }

    lai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = LAI_NULL_OBJECT_ID } } };

    status = meta_generic_validation_create(meta_key, linecard_id, attr_count, attr_list);

    if (status != LAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = m_implementation->create(object_type, object_id, linecard_id, attr_count, attr_list);

    if (status == LAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("create status: %s", lai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("create status: %s", lai_serialize_status(status).c_str());
    }

    if (status == LAI_STATUS_SUCCESS)
    {
        meta_key.objectkey.key.object_id = *object_id;

        if (meta_key.objecttype == LAI_OBJECT_TYPE_LINECARD)
        {
            /*
             * We are creating linecard object, so linecard id must be the same as
             * just created object. We could use LAI_NULL_OBJECT_ID in that
             * case and do special linecard inside post_create method.
             */

            linecard_id = *object_id;
        }

        meta_generic_validation_post_create(meta_key, linecard_id, attr_count, attr_list);
    }

    return status;
}

lai_status_t Meta::set(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    lai_status_t status = meta_lai_validate_oid(object_type, &object_id, LAI_NULL_OBJECT_ID, false);

    if (status != LAI_STATUS_SUCCESS)
    {
        return status;
    }

    lai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = object_id } } };

    status = meta_generic_validation_set(meta_key, attr);

    if (status != LAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = m_implementation->set(object_type, object_id, attr);

    if (status == LAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("set status: %s", lai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("set status: %s", lai_serialize_status(status).c_str());
    }

    if (status == LAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_set(meta_key, attr);
    }

    return status;
}

lai_status_t Meta::get(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t attr_count,
        _Inout_ lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    lai_status_t status = meta_lai_validate_oid(object_type, &object_id, LAI_NULL_OBJECT_ID, false);

    if (status != LAI_STATUS_SUCCESS)
    {
        return status;
    }

    lai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = object_id } } };

    status = meta_generic_validation_get(meta_key, attr_count, attr_list);

    if (status != LAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = m_implementation->get(object_type, object_id, attr_count, attr_list);

    if (status == LAI_STATUS_SUCCESS)
    {
        lai_object_id_t linecard_id = linecardIdQuery(object_id);

        if (!m_oids.objectReferenceExists(linecard_id))
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist", linecard_id);
        }

        meta_generic_validation_post_get(meta_key, linecard_id, attr_count, attr_list);
    }

    return status;
}

#define PARAMETER_CHECK_IF_NOT_NULL(param) {                                                \
    if ((param) == nullptr) {                                                               \
        SWSS_LOG_ERROR("parameter " # param " is NULL");                                    \
        return LAI_STATUS_INVALID_PARAMETER; } }

#define PARAMETER_CHECK_OID_OBJECT_TYPE(param, OT) {                                        \
    lai_object_type_t _ot = objectTypeQuery(param);                                   \
    if (_ot != (OT)) {                                                                      \
        SWSS_LOG_ERROR("parameter " # param " %s object type is %s, but expected %s",       \
                lai_serialize_object_id(param).c_str(),                                     \
                lai_serialize_object_type(_ot).c_str(),                                     \
                lai_serialize_object_type(OT).c_str());                                     \
        return LAI_STATUS_INVALID_PARAMETER; } }

#define PARAMETER_CHECK_OBJECT_TYPE_VALID(ot) {                                             \
    if (!lai_metadata_is_object_type_valid(ot)) {                                           \
        SWSS_LOG_ERROR("parameter " # ot " object type %d is invalid", (ot));               \
        return LAI_STATUS_INVALID_PARAMETER; } }

#define PARAMETER_CHECK_POSITIVE(param) {                                                   \
    if ((param) <= 0) {                                                                     \
        SWSS_LOG_ERROR("parameter " #param " must be positive");                            \
        return LAI_STATUS_INVALID_PARAMETER; } }

#define PARAMETER_CHECK_OID_EXISTS(oid, OT) {                                               \
    lai_object_meta_key_t _key = {                                                          \
        .objecttype = (OT), .objectkey = { .key = { .object_id = (oid) } } };               \
    if (!m_laiObjectCollection.objectExists(_key)) {                                        \
        SWSS_LOG_ERROR("object %s don't exists", lai_serialize_object_id(oid).c_str()); } }

lai_status_t Meta::objectTypeGetAvailability(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ uint32_t attrCount,
        _In_ const lai_attribute_t *attrList,
        _Out_ uint64_t *count)
{
    SWSS_LOG_ENTER();

    PARAMETER_CHECK_OID_OBJECT_TYPE(linecardId, LAI_OBJECT_TYPE_LINECARD);
    PARAMETER_CHECK_OID_EXISTS(linecardId, LAI_OBJECT_TYPE_LINECARD);
    PARAMETER_CHECK_OBJECT_TYPE_VALID(objectType);
    PARAMETER_CHECK_POSITIVE(attrCount);
    PARAMETER_CHECK_IF_NOT_NULL(attrList);
    PARAMETER_CHECK_IF_NOT_NULL(count);

    auto info = lai_metadata_get_object_type_info(objectType);

    PARAMETER_CHECK_IF_NOT_NULL(info);

    std::set<lai_attr_id_t> attrs;

    for (uint32_t idx = 0; idx < attrCount; idx++)
    {
        auto id = attrList[idx].id;

        auto mdp = lai_metadata_get_attr_metadata(objectType, id);

        if (mdp == nullptr)
        {
            SWSS_LOG_ERROR("can't find attribute %s:%d",
                    info->objecttypename,
                    attrList[idx].id);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (attrs.find(id) != attrs.end())
        {
            SWSS_LOG_ERROR("attr %s already defined on list", mdp->attridname);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        attrs.insert(id);

        if (!mdp->isresourcetype)
        {
            SWSS_LOG_ERROR("attr %s is not resource type", mdp->attridname);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        switch (mdp->attrvaluetype)
        {
            case LAI_ATTR_VALUE_TYPE_INT32:

                if (mdp->isenum && !lai_metadata_is_allowed_enum_value(mdp, attrList[idx].value.s32))
                {
                    SWSS_LOG_ERROR("%s is enum, but value %d not found on allowed values list",
                            mdp->attridname,
                            attrList[idx].value.s32);

                    return LAI_STATUS_INVALID_PARAMETER;
                }

                break;

            default:

                SWSS_LOG_THROW("value type %s not supported yet, FIXME!",
                        lai_serialize_attr_value_type(mdp->attrvaluetype).c_str());
        }
    }

    auto status = m_implementation->objectTypeGetAvailability(linecardId, objectType, attrCount, attrList, count);

    // no post validation required

    return status;
}

lai_status_t Meta::queryAttributeCapability(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ lai_attr_id_t attrId,
        _Out_ lai_attr_capability_t *capability)
{
    SWSS_LOG_ENTER();

    PARAMETER_CHECK_OID_OBJECT_TYPE(linecardId, LAI_OBJECT_TYPE_LINECARD);
    PARAMETER_CHECK_OID_EXISTS(linecardId, LAI_OBJECT_TYPE_LINECARD);
    PARAMETER_CHECK_OBJECT_TYPE_VALID(objectType);

    auto mdp = lai_metadata_get_attr_metadata(objectType, attrId);

    if (!mdp)
    {
        SWSS_LOG_ERROR("unable to find attribute: %s:%d",
                lai_serialize_object_type(objectType).c_str(),
                attrId);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    PARAMETER_CHECK_IF_NOT_NULL(capability);

    auto status = m_implementation->queryAttributeCapability(linecardId, objectType, attrId, capability);

    return status;
}

lai_status_t Meta::queryAattributeEnumValuesCapability(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ lai_attr_id_t attrId,
        _Inout_ lai_s32_list_t *enumValuesCapability)
{
    SWSS_LOG_ENTER();

    PARAMETER_CHECK_OID_OBJECT_TYPE(linecardId, LAI_OBJECT_TYPE_LINECARD);
    PARAMETER_CHECK_OID_EXISTS(linecardId, LAI_OBJECT_TYPE_LINECARD);
    PARAMETER_CHECK_OBJECT_TYPE_VALID(objectType);

    auto mdp = lai_metadata_get_attr_metadata(objectType, attrId);

    if (!mdp)
    {
        SWSS_LOG_ERROR("unable to find attribute: %s:%d",
                lai_serialize_object_type(objectType).c_str(),
                attrId);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    if (!mdp->isenum && !mdp->isenumlist)
    {
        SWSS_LOG_ERROR("%s is not enum/enum list", mdp->attridname);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    PARAMETER_CHECK_IF_NOT_NULL(enumValuesCapability);

    if (meta_genetic_validation_list(*mdp, enumValuesCapability->count, enumValuesCapability->list)
            != LAI_STATUS_SUCCESS)
    {
        return LAI_STATUS_INVALID_PARAMETER;
    }

    auto status = m_implementation->queryAattributeEnumValuesCapability(linecardId, objectType, attrId, enumValuesCapability);

    if (status == LAI_STATUS_SUCCESS)
    {
        if (enumValuesCapability->list)
        {
            // check if all returned values are members of defined enum
            for (uint32_t idx = 0; idx < enumValuesCapability->count; idx++)
            {
                int val = enumValuesCapability->list[idx];

                if (!lai_metadata_is_allowed_enum_value(mdp, val))
                {
                    SWSS_LOG_ERROR("returned value %d is not allowed on %s", val, mdp->attridname);
                }
            }
        }
    }

    return status;
}

#define META_COUNTERS_COUNT_MSB (0x80000000)

lai_status_t Meta::meta_validate_stats(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids,
        _Out_ lai_stat_value_t *counters,
        _In_ lai_stats_mode_t mode)
{
    SWSS_LOG_ENTER();

    /*
     * If last bit of counters count is set to high, and unittests are enabled,
     * then this api can be used to SET counter values by user for debugging purposes.
     */

    if (m_unittestsEnabled)
    {
        number_of_counters &= ~(META_COUNTERS_COUNT_MSB);
    }

    PARAMETER_CHECK_OBJECT_TYPE_VALID(object_type);
    PARAMETER_CHECK_OID_OBJECT_TYPE(object_id, object_type);
    PARAMETER_CHECK_OID_EXISTS(object_id, object_type);
    PARAMETER_CHECK_POSITIVE(number_of_counters);
    PARAMETER_CHECK_IF_NOT_NULL(counter_ids);
    PARAMETER_CHECK_IF_NOT_NULL(counters);

    lai_object_id_t linecard_id = linecardIdQuery(object_id);

    // checks also if object type is OID
    lai_status_t status = meta_lai_validate_oid(object_type, &object_id, linecard_id, false);

    CHECK_STATUS_SUCCESS(status);

    auto info = lai_metadata_get_object_type_info(object_type);

    PARAMETER_CHECK_IF_NOT_NULL(info);

    if (info->statenum == nullptr)
    {
        SWSS_LOG_ERROR("%s does not support stats", info->objecttypename);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    // check if all counter ids are in enum range

    for (uint32_t idx = 0; idx < number_of_counters; idx++)
    {
        if (lai_metadata_get_enum_value_name(info->statenum, counter_ids[idx]) == nullptr)
        {
            SWSS_LOG_ERROR("value %d is not in range on %s", counter_ids[idx], info->statenum->name);

            return LAI_STATUS_INVALID_PARAMETER;
        }
    }

    // check mode

    if (lai_metadata_get_enum_value_name(&lai_metadata_enum_lai_stats_mode_t, mode) == nullptr)
    {
        SWSS_LOG_ERROR("mode value %d is not in range on %s", mode, lai_metadata_enum_lai_stats_mode_t.name);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    return LAI_STATUS_SUCCESS;
}

lai_status_t Meta::getStats(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids,
        _Out_ lai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    auto status = meta_validate_stats(object_type, object_id, number_of_counters, counter_ids, counters, LAI_STATS_MODE_READ);

    CHECK_STATUS_SUCCESS(status);

    status = m_implementation->getStats(object_type, object_id, number_of_counters, counter_ids, counters);

    // no post validation required

    return status;
}

lai_status_t Meta::getStatsExt(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids,
        _In_ lai_stats_mode_t mode,
        _Out_ lai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    auto status = meta_validate_stats(object_type, object_id, number_of_counters, counter_ids, counters, mode);

    CHECK_STATUS_SUCCESS(status);

    status = m_implementation->getStatsExt(object_type, object_id, number_of_counters, counter_ids, mode, counters);

    // no post validation required

    return status;
}

lai_status_t Meta::clearStats(
        _In_ lai_object_type_t object_type,
        _In_ lai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const lai_stat_id_t *counter_ids)
{
    SWSS_LOG_ENTER();

    lai_stat_value_t counters;
    auto status = meta_validate_stats(object_type, object_id, number_of_counters, counter_ids, &counters, LAI_STATS_MODE_READ);

    CHECK_STATUS_SUCCESS(status);

    status = m_implementation->clearStats(object_type, object_id, number_of_counters, counter_ids);

    // no post validation required

    return status;
}

lai_object_type_t Meta::objectTypeQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_implementation->objectTypeQuery(objectId);
}

lai_object_id_t Meta::linecardIdQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_implementation->linecardIdQuery(objectId);
}

lai_status_t Meta::logSet(
        _In_ lai_api_t api,
        _In_ lai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    // TODO check api and log level

    return m_implementation->logSet(api, log_level);
}

void Meta::clean_after_linecard_remove(
        _In_ lai_object_id_t linecardId)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("cleaning metadata for linecard: %s",
            lai_serialize_object_id(linecardId).c_str());

    if (objectTypeQuery(linecardId) != LAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_THROW("oid %s is not LINECARD!",
                lai_serialize_object_id(linecardId).c_str());
    }

    // clear port related objects

    for (auto port: m_portRelatedSet.getAllPorts())
    {
        if (linecardIdQuery(port) == linecardId)
        {
            m_portRelatedSet.removePort(port);
        }
    }

    // clear oid references

    for (auto oid: m_oids.getAllOids())
    {
        if (linecardIdQuery(oid) == linecardId)
        {
            m_oids.objectReferenceClear(oid);
        }
    }

    // clear attr keys

    for (auto& key: m_attrKeys.getAllKeys())
    {
        lai_object_meta_key_t mk;
        lai_deserialize_object_meta_key(key, mk);

        // we guarantee that linecard_id is first in the key structure so we can
        // use that as object_id as well

        if (linecardIdQuery(mk.objectkey.key.object_id) == linecardId)
        {
            m_attrKeys.eraseMetaKey(key);
        }
    }

    for (auto& mk: m_laiObjectCollection.getAllKeys())
    {
        // we guarantee that linecard_id is first in the key structure so we can
        // use that as object_id as well

        if (linecardIdQuery(mk.objectkey.key.object_id) == linecardId)
        {
            m_laiObjectCollection.removeObject(mk);
        }
    }

    SWSS_LOG_NOTICE("removed all objects related to linecard %s",
            lai_serialize_object_id(linecardId).c_str());
}

lai_status_t Meta::meta_generic_validation_remove(
        _In_ const lai_object_meta_key_t& meta_key)
{
    SWSS_LOG_ENTER();

    if (!m_laiObjectCollection.objectExists(meta_key))
    {
        SWSS_LOG_ERROR("object key %s doesn't exist",
                lai_serialize_object_meta_key(meta_key).c_str());

        return LAI_STATUS_INVALID_PARAMETER;
    }

    auto info = lai_metadata_get_object_type_info(meta_key.objecttype);

    if (info->isnonobjectid)
    {
        // we don't keep reference of those since those are leafs
        return LAI_STATUS_SUCCESS;
    }

    // for OID objects check oid value

    lai_object_id_t oid = meta_key.objectkey.key.object_id;

    if (oid == LAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_ERROR("can't remove null object id");

        return LAI_STATUS_INVALID_PARAMETER;
    }

    lai_object_type_t object_type = objectTypeQuery(oid);

    if (object_type == LAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_ERROR("oid 0x%" PRIx64 " is not valid, returned null object id", oid);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    if (object_type != meta_key.objecttype)
    {
        SWSS_LOG_ERROR("oid 0x%" PRIx64 " type %d is not accepted, expected object type %d", oid, object_type, meta_key.objecttype);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    if (!m_oids.objectReferenceExists(oid))
    {
        SWSS_LOG_ERROR("object 0x%" PRIx64 " reference doesn't exist", oid);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    int count = m_oids.getObjectReferenceCount(oid);

    if (count != 0)
    {
        if (object_type == LAI_OBJECT_TYPE_LINECARD)
        {
            /*
             * We allow to remove linecard object even if there are ROUTE_ENTRY
             * created and referencing this linecard, since remove could be used
             * in WARM boot scenario.
             */

            SWSS_LOG_WARN("removing linecard object 0x%" PRIx64 " reference count is %d, removing all objects from meta DB", oid, count);

            return LAI_STATUS_SUCCESS;
        }

        SWSS_LOG_ERROR("object 0x%" PRIx64 " reference count is %d, can't remove", oid, count);

        return LAI_STATUS_OBJECT_IN_USE;
    }

    // should be safe to remove

    return LAI_STATUS_SUCCESS;
}

lai_status_t Meta::meta_port_remove_validation(
        _In_ const lai_object_meta_key_t& meta_key)
{
    SWSS_LOG_ENTER();

    lai_object_id_t port_id = meta_key.objectkey.key.object_id;

    auto relatedObjects = m_portRelatedSet.getPortRelatedObjects(port_id);

    if (relatedObjects.size() == 0)
    {
        // user didn't query any queues, ipgs or scheduler groups
        // for this port, then we can just skip this
        return LAI_STATUS_SUCCESS;
    }

    if (!meta_is_object_in_default_state(port_id))
    {
        SWSS_LOG_ERROR("port %s is not in default state, can't remove",
                lai_serialize_object_id(port_id).c_str());

        return LAI_STATUS_OBJECT_IN_USE;
    }

    for (auto oid: relatedObjects)
    {
        if (m_oids.getObjectReferenceCount(oid) != 0)
        {
            SWSS_LOG_ERROR("port %s related object %s reference count is not zero, can't remove",
                    lai_serialize_object_id(port_id).c_str(),
                    lai_serialize_object_id(oid).c_str());

            return LAI_STATUS_OBJECT_IN_USE;
        }

        if (!meta_is_object_in_default_state(oid))
        {
            SWSS_LOG_ERROR("port related object %s is not in default state, can't remove",
                    lai_serialize_object_id(oid).c_str());

            return LAI_STATUS_OBJECT_IN_USE;
        }
    }

    SWSS_LOG_NOTICE("all objects related to port %s are in default state, can be remove",
                lai_serialize_object_id(port_id).c_str());

    return LAI_STATUS_SUCCESS;
}

bool Meta::meta_is_object_in_default_state(
        _In_ lai_object_id_t oid)
{
    SWSS_LOG_ENTER();

    if (oid == LAI_NULL_OBJECT_ID)
        SWSS_LOG_THROW("not expected NULL object id");

    if (!m_oids.objectReferenceExists(oid))
    {
        SWSS_LOG_WARN("object %s reference not exists, bug!",
                lai_serialize_object_id(oid).c_str());
        return false;
    }

    lai_object_meta_key_t meta_key;

    meta_key.objecttype = objectTypeQuery(oid);
    meta_key.objectkey.key.object_id = oid;

    if (!m_laiObjectCollection.objectExists(meta_key))
    {
        SWSS_LOG_WARN("object %s don't exists in local database, bug!",
                lai_serialize_object_id(oid).c_str());
        return false;
    }

    auto attrs = m_laiObjectCollection.getObject(meta_key)->getAttributes();

    for (const auto& attr: attrs)
    {
        auto &md = *attr->getLaiAttrMetadata();

        auto *a = attr->getLaiAttr();

        if (md.isreadonly)
            continue;

        if (!md.isoidattribute)
            continue;

        if (md.attrvaluetype == LAI_ATTR_VALUE_TYPE_OBJECT_ID)
        {
            if (a->value.oid != LAI_NULL_OBJECT_ID)
            {
                SWSS_LOG_ERROR("object %s has non default state on %s: %s, expected NULL",
                        lai_serialize_object_id(oid).c_str(),
                        md.attridname,
                        lai_serialize_object_id(a->value.oid).c_str());

                return false;
            }
        }
        else if (md.attrvaluetype == LAI_ATTR_VALUE_TYPE_OBJECT_LIST)
        {
            for (uint32_t i = 0; i < a->value.objlist.count; i++)
            {
                if (a->value.objlist.list[i] != LAI_NULL_OBJECT_ID)
                {
                    SWSS_LOG_ERROR("object %s has non default state on %s[%u]: %s, expected NULL",
                            lai_serialize_object_id(oid).c_str(),
                            md.attridname,
                            i,
                            lai_serialize_object_id(a->value.objlist.list[i]).c_str());

                    return false;
                }
            }
        }
        else
        {
            // unable to check whether object is in default state, need fix

            SWSS_LOG_ERROR("unsupported oid attribute: %s, FIX ME!", md.attridname);
            return false;
        }
    }

    return true;
}

lai_status_t Meta::meta_lai_validate_oid(
        _In_ lai_object_type_t object_type,
        _In_ const lai_object_id_t* object_id,
        _In_ lai_object_id_t linecard_id,
        _In_ bool bcreate)
{
    SWSS_LOG_ENTER();

    if (object_type <= LAI_OBJECT_TYPE_NULL ||
            object_type >= LAI_OBJECT_TYPE_EXTENSIONS_MAX)
    {
        SWSS_LOG_ERROR("invalid object type specified: %d, FIXME", object_type);
        return LAI_STATUS_INVALID_PARAMETER;
    }

    const char* otname =  lai_metadata_get_enum_value_name(&lai_metadata_enum_lai_object_type_t, object_type);

    auto info = lai_metadata_get_object_type_info(object_type);

    if (info->isnonobjectid)
    {
        SWSS_LOG_THROW("invalid object type (%s) specified as generic, FIXME", otname);
    }

    SWSS_LOG_DEBUG("generic object type: %s", otname);

    if (object_id == NULL)
    {
        SWSS_LOG_ERROR("oid pointer is NULL");

        return LAI_STATUS_INVALID_PARAMETER;
    }

    if (bcreate)
    {
        return LAI_STATUS_SUCCESS;
    }

    lai_object_id_t oid = *object_id;

    if (oid == LAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_ERROR("oid is set to null object id on %s", otname);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    lai_object_type_t ot = objectTypeQuery(oid);

    if (ot == LAI_OBJECT_TYPE_NULL)
    {
        SWSS_LOG_ERROR("%s oid 0x%" PRIx64 " is not valid object type, returned null object type", otname, oid);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    lai_object_type_t expected = object_type;

    if (ot != expected)
    {
        SWSS_LOG_ERROR("%s oid 0x%" PRIx64 " type %d is wrong type, expected object type %d", otname, oid, ot, expected);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    // check if object exists

    lai_object_meta_key_t meta_key_oid = { .objecttype = expected, .objectkey = { .key = { .object_id = oid } } };

    if (!m_laiObjectCollection.objectExists(meta_key_oid))
    {
        SWSS_LOG_ERROR("object key %s doesn't exist",
                lai_serialize_object_meta_key(meta_key_oid).c_str());

        return LAI_STATUS_INVALID_PARAMETER;
    }

    return LAI_STATUS_SUCCESS;
}

void Meta::meta_generic_validation_post_remove(
        _In_ const lai_object_meta_key_t& meta_key)
{
    SWSS_LOG_ENTER();

    if (meta_key.objecttype == LAI_OBJECT_TYPE_LINECARD)
    {
        /*
         * If linecard object was removed then meta db was cleared and there are
         * no other attributes, no need for reference counting.
         */

        clean_after_linecard_remove(meta_key.objectkey.key.object_id);

        return;
    }

    // get all attributes that was set

    for (auto&it: m_laiObjectCollection.getObject(meta_key)->getAttributes())
    {
        const lai_attribute_t* attr = it->getLaiAttr();

        auto mdp = lai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

        const lai_attribute_value_t& value = attr->value;

        const lai_attr_metadata_t& md = *mdp;

        // decrease reference on object id types

        switch (md.attrvaluetype)
        {
            case LAI_ATTR_VALUE_TYPE_BOOL:
            case LAI_ATTR_VALUE_TYPE_CHARDATA:
            case LAI_ATTR_VALUE_TYPE_UINT8:
            case LAI_ATTR_VALUE_TYPE_INT8:
            case LAI_ATTR_VALUE_TYPE_UINT16:
            case LAI_ATTR_VALUE_TYPE_INT16:
            case LAI_ATTR_VALUE_TYPE_UINT32:
            case LAI_ATTR_VALUE_TYPE_INT32:
            case LAI_ATTR_VALUE_TYPE_UINT64:
            case LAI_ATTR_VALUE_TYPE_INT64:
            case LAI_ATTR_VALUE_TYPE_DOUBLE:
            case LAI_ATTR_VALUE_TYPE_POINTER:
                // primitives, ok
                break;

            case LAI_ATTR_VALUE_TYPE_OBJECT_ID:
                m_oids.objectReferenceDecrement(value.oid);
                break;

            case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:
                m_oids.objectReferenceDecrement(value.objlist);
                break;

            case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
            case LAI_ATTR_VALUE_TYPE_INT8_LIST:
            case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
            case LAI_ATTR_VALUE_TYPE_INT16_LIST:
            case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
            case LAI_ATTR_VALUE_TYPE_INT32_LIST:
            case LAI_ATTR_VALUE_TYPE_UINT32_RANGE:
            case LAI_ATTR_VALUE_TYPE_INT32_RANGE:
                // no special action required
                break;

            default:
                META_LOG_THROW(md, "serialization type is not supported yet FIXME");
        }
    }

    // we don't keep track of fdb, neighbor, route since
    // those are safe to remove any time (leafs)

    auto info = lai_metadata_get_object_type_info(meta_key.objecttype);

    if (info->isnonobjectid)
    {
        /*
         * Decrease object reference count for all object ids in non object id
         * members.
         */

        for (size_t j = 0; j < info->structmemberscount; ++j)
        {
            const lai_struct_member_info_t *m = info->structmembers[j];

            if (m->membervaluetype != LAI_ATTR_VALUE_TYPE_OBJECT_ID)
            {
                continue;
            }

            m_oids.objectReferenceDecrement(m->getoid(&meta_key));
        }
    }
    else
    {
        m_oids.objectReferenceRemove(meta_key.objectkey.key.object_id);
    }

    m_laiObjectCollection.removeObject(meta_key);

    std::string metaKey = lai_serialize_object_meta_key(meta_key);

    m_attrKeys.eraseMetaKey(metaKey);

}

lai_status_t Meta::meta_generic_validation_create(
        _In_ const lai_object_meta_key_t& meta_key,
        _In_ lai_object_id_t linecard_id,
        _In_ const uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (attr_count > MAX_LIST_COUNT)
    {
        SWSS_LOG_ERROR("create attribute count %u > max list count %u", attr_count, MAX_LIST_COUNT);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    if (attr_count > 0 && attr_list == NULL)
    {
        SWSS_LOG_ERROR("attr count is %u but attribute list pointer is NULL", attr_count);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    bool linecardcreate = meta_key.objecttype == LAI_OBJECT_TYPE_LINECARD;

    if (linecardcreate)
    {
        // we are creating linecard

        linecard_id = LAI_NULL_OBJECT_ID;

        /*
         * Creating linecard can't have any object attributes set on it, OID
         * attributes must be applied on linecard using SET API.
         */

        for (uint32_t i = 0; i < attr_count; ++i)
        {
            auto meta = lai_metadata_get_attr_metadata(LAI_OBJECT_TYPE_LINECARD, attr_list[i].id);

            if (meta == NULL)
            {
                SWSS_LOG_ERROR("attribute %d not found", attr_list[i].id);

                return LAI_STATUS_INVALID_PARAMETER;
            }

            if (meta->isoidattribute)
            {
                SWSS_LOG_ERROR("%s is OID attribute, not allowed on create linecard", meta->attridname);

                return LAI_STATUS_INVALID_PARAMETER;
            }
        }
    }
    else
    {
        /*
         * Non linecard object case (also non object id)
         *
         * NOTE: this is a lot of checks for each create
         */

        linecard_id = meta_extract_linecard_id(meta_key, linecard_id);

        if (linecard_id == LAI_NULL_OBJECT_ID)
        {
            SWSS_LOG_ERROR("linecard id is NULL for %s", lai_serialize_object_type(meta_key.objecttype).c_str());

            return LAI_STATUS_INVALID_PARAMETER;
        }

        lai_object_type_t sw_type = objectTypeQuery(linecard_id);

        if (sw_type != LAI_OBJECT_TYPE_LINECARD)
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " type is %s, expected LINECARD", linecard_id, lai_serialize_object_type(sw_type).c_str());

            return LAI_STATUS_INVALID_PARAMETER;
        }

        // check if linecard exists

        lai_object_meta_key_t linecard_meta_key = { .objecttype = LAI_OBJECT_TYPE_LINECARD, .objectkey = { .key = { .object_id = linecard_id } } };

        if (!m_laiObjectCollection.objectExists(linecard_meta_key))
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist yet", linecard_id);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (!m_oids.objectReferenceExists(linecard_id))
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist yet", linecard_id);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        // ok
    }

    lai_status_t status = meta_generic_validate_non_object_on_create(meta_key, linecard_id);

    if (status != LAI_STATUS_SUCCESS)
    {
        return status;
    }

    std::unordered_map<lai_attr_id_t, const lai_attribute_t*> attrs;

    SWSS_LOG_DEBUG("attr count = %u", attr_count);

    bool haskeys = false;

    // check each attribute separately
    for (uint32_t idx = 0; idx < attr_count; ++idx)
    {
        const lai_attribute_t* attr = &attr_list[idx];

        auto mdp = lai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

        if (mdp == NULL)
        {
            SWSS_LOG_ERROR("unable to find attribute metadata %d:%d", meta_key.objecttype, attr->id);

            return LAI_STATUS_FAILURE;
        }

        const lai_attribute_value_t& value = attr->value;

        const lai_attr_metadata_t& md = *mdp;

        META_LOG_DEBUG(md, "(create)");

        if (attrs.find(attr->id) != attrs.end())
        {
            META_LOG_ERROR(md, "attribute id (%u) is defined on attr list multiple times", attr->id);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        attrs[attr->id] = attr;

        if (LAI_HAS_FLAG_READ_ONLY(md.flags))
        {
            META_LOG_ERROR(md, "attr is read only and cannot be created");

            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (LAI_HAS_FLAG_KEY(md.flags))
        {
            haskeys = true;

            META_LOG_DEBUG(md, "attr is key");
        }

        // if we set OID check if exists and if type is correct
        // and it belongs to the same linecard id

        switch (md.attrvaluetype)
        {
            case LAI_ATTR_VALUE_TYPE_BOOL:
            case LAI_ATTR_VALUE_TYPE_UINT8:
            case LAI_ATTR_VALUE_TYPE_INT8:
            case LAI_ATTR_VALUE_TYPE_UINT16:
            case LAI_ATTR_VALUE_TYPE_INT16:
            case LAI_ATTR_VALUE_TYPE_UINT32:
            case LAI_ATTR_VALUE_TYPE_INT32:
            case LAI_ATTR_VALUE_TYPE_UINT64:
            case LAI_ATTR_VALUE_TYPE_INT64:
            case LAI_ATTR_VALUE_TYPE_DOUBLE:
            case LAI_ATTR_VALUE_TYPE_POINTER:
            case LAI_ATTR_VALUE_TYPE_CHARDATA:
                // primitives
                break;

            case LAI_ATTR_VALUE_TYPE_OBJECT_ID:

                {
                    status = meta_generic_validation_objlist(md, linecard_id, 1, &value.oid);

                    if (status != LAI_STATUS_SUCCESS)
                    {
                        return status;
                    }

                    break;
                }

            case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:

                {
                    status = meta_generic_validation_objlist(md, linecard_id, value.objlist.count, value.objlist.list);

                    if (status != LAI_STATUS_SUCCESS)
                    {
                        return status;
                    }

                    break;
                }

            case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
                VALIDATION_LIST(md, value.u8list);
                break;
            case LAI_ATTR_VALUE_TYPE_INT8_LIST:
                VALIDATION_LIST(md, value.s8list);
                break;
            case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
                VALIDATION_LIST(md, value.u16list);
                break;
            case LAI_ATTR_VALUE_TYPE_INT16_LIST:
                VALIDATION_LIST(md, value.s16list);
                break;
            case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
                VALIDATION_LIST(md, value.u32list);
                break;
            case LAI_ATTR_VALUE_TYPE_INT32_LIST:
                VALIDATION_LIST(md, value.s32list);
                break;

            case LAI_ATTR_VALUE_TYPE_UINT32_RANGE:

                if (value.u32range.min > value.u32range.max)
                {
                    META_LOG_ERROR(md, "invalid range %u .. %u", value.u32range.min, value.u32range.max);

                    return LAI_STATUS_INVALID_PARAMETER;
                }

                break;

            case LAI_ATTR_VALUE_TYPE_INT32_RANGE:

                if (value.s32range.min > value.s32range.max)
                {
                    META_LOG_ERROR(md, "invalid range %u .. %u", value.s32range.min, value.s32range.max);

                    return LAI_STATUS_INVALID_PARAMETER;
                }

                break;

            default:

                META_LOG_THROW(md, "serialization type is not supported yet FIXME");
        }

        if (md.isenum)
        {
            int32_t val = value.s32;

            val = value.s32;

            if (!lai_metadata_is_allowed_enum_value(&md, val))
            {
                META_LOG_ERROR(md, "is enum, but value %d not found on allowed values list", val);

                return LAI_STATUS_INVALID_PARAMETER;
            }
        }

        if (md.isenumlist)
        {
            // we allow repeats on enum list
            if (value.s32list.count != 0 && value.s32list.list == NULL)
            {
                META_LOG_ERROR(md, "enum list is NULL");

                return LAI_STATUS_INVALID_PARAMETER;
            }

            for (uint32_t i = value.s32list.count; i < value.s32list.count; ++i)
            {
                int32_t s32 = value.s32list.list[i];

                if (!lai_metadata_is_allowed_enum_value(&md, s32))
                {
                    META_LOG_ERROR(md, "is enum list, but value %d not found on allowed values list", s32);

                    return LAI_STATUS_INVALID_PARAMETER;
                }
            }
        }

        // conditions are checked later on
    }

    // we are creating object, no need for check if exists (only key values needs to be checked)

    auto info = lai_metadata_get_object_type_info(meta_key.objecttype);

    if (info->isnonobjectid)
    {
        // just sanity check if object already exists

        if (m_laiObjectCollection.objectExists(meta_key))
        {
            SWSS_LOG_ERROR("object key %s already exists",
                    lai_serialize_object_meta_key(meta_key).c_str());

            return LAI_STATUS_ITEM_ALREADY_EXISTS;
        }
    }
    else
    {
        /*
         * We are creating OID object, and we don't have it's value yet so we
         * can't do any check on it.
         */
    }

    const auto& metadata = get_attributes_metadata(meta_key.objecttype);

    if (metadata.empty())
    {
        SWSS_LOG_ERROR("get attributes metadata returned empty list for object type: %d", meta_key.objecttype);

        return LAI_STATUS_FAILURE;
    }

    // check if all mandatory attributes were passed

    for (auto mdp: metadata)
    {
        const lai_attr_metadata_t& md = *mdp;

        if (!LAI_HAS_FLAG_MANDATORY_ON_CREATE(md.flags))
        {
            continue;
        }

        if (md.isconditional)
        {
            // skip conditional attributes for now
            continue;
        }

        const auto &it = attrs.find(md.attrid);

        if (it == attrs.end())
        {
            /*
             * Buffer profile shared static/dynamic is special case since it's
             * mandatory on create but condition is on
             * LAI_BUFFER_PROFILE_ATTR_POOL_ID attribute (see file laibuffer.h).
             */

            META_LOG_ERROR(md, "attribute is mandatory but not passed in attr list");

            return LAI_STATUS_MANDATORY_ATTRIBUTE_MISSING;
        }
    }

    // check if we need any conditional attributes
    for (auto mdp: metadata)
    {
        const lai_attr_metadata_t& md = *mdp;

        if (!md.isconditional)
        {
            continue;
        }

        // this is conditional attribute, check if it's required

        bool any = false;

        for (size_t index = 0; md.conditions[index] != NULL; index++)
        {
            const auto& c = *md.conditions[index];

            // conditions may only be on the same object type
            const auto& cmd = *lai_metadata_get_attr_metadata(meta_key.objecttype, c.attrid);

            const lai_attribute_value_t* cvalue = cmd.defaultvalue;

            const lai_attribute_t *cattr = lai_metadata_get_attr_by_id(c.attrid, attr_count, attr_list);

            if (cattr != NULL)
            {
                META_LOG_DEBUG(md, "condition attr %d was passed, using it's value", c.attrid);

                cvalue = &cattr->value;
            }

            if (cmd.attrvaluetype == LAI_ATTR_VALUE_TYPE_BOOL)
            {
                if (c.condition.booldata == cvalue->booldata)
                {
                    META_LOG_DEBUG(md, "bool condition was met on attr %d = %d", cmd.attrid, c.condition.booldata);

                    any = true;
                    break;
                }
            }
            else // enum condition
            {
                int32_t val = cvalue->s32;

                val = cvalue->s32;

                if (c.condition.s32 == val)
                {
                    META_LOG_DEBUG(md, "enum condition was met on attr id %d, val = %d", cmd.attrid, val);

                    any = true;
                    break;
                }
            }
        }

        if (!any)
        {
            // maybe we can let it go here?
            if (attrs.find(md.attrid) != attrs.end())
            {
                META_LOG_ERROR(md, "conditional, but condition was not met, this attribute is not required, but passed");

                return LAI_STATUS_INVALID_PARAMETER;
            }

            continue;
        }

        // is required, check if user passed it
        const auto &it = attrs.find(md.attrid);

        if (it == attrs.end())
        {
            META_LOG_ERROR(md, "attribute is conditional and is mandatory but not passed in attr list");

            return LAI_STATUS_MANDATORY_ATTRIBUTE_MISSING;
        }
    }

    if (haskeys)
    {
        std::string key = AttrKeyMap::constructKey(meta_key, attr_count, attr_list);

        // since we didn't created oid yet, we don't know if attribute key exists, check all
        if (m_attrKeys.attrKeyExists(key))
        {
            SWSS_LOG_ERROR("attribute key %s already exists, can't create", key.c_str());

            return LAI_STATUS_INVALID_PARAMETER;
        }
    }

    return LAI_STATUS_SUCCESS;
}

lai_status_t Meta::meta_generic_validation_set(
        _In_ const lai_object_meta_key_t& meta_key,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    if (attr == NULL)
    {
        SWSS_LOG_ERROR("attribute pointer is NULL");

        return LAI_STATUS_INVALID_PARAMETER;
    }

    auto mdp = lai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

    if (mdp == NULL)
    {
        SWSS_LOG_ERROR("unable to find attribute metadata %d:%d", meta_key.objecttype, attr->id);

        return LAI_STATUS_FAILURE;
    }

    const lai_attribute_value_t& value = attr->value;

    const lai_attr_metadata_t& md = *mdp;

    META_LOG_DEBUG(md, "(set)");

    if (LAI_HAS_FLAG_READ_ONLY(md.flags))
    {
        if (meta_unittests_get_and_erase_set_readonly_flag(md))
        {
            META_LOG_NOTICE(md, "readonly attribute is allowed to be set (unittests enabled)");
        }
        else
        {
            META_LOG_ERROR(md, "attr is read only and cannot be modified");

            return LAI_STATUS_INVALID_PARAMETER;
        }
    }

    if (LAI_HAS_FLAG_CREATE_ONLY(md.flags))
    {
        META_LOG_ERROR(md, "attr is create only and cannot be modified");

        return LAI_STATUS_INVALID_PARAMETER;
    }

    if (LAI_HAS_FLAG_KEY(md.flags))
    {
        META_LOG_ERROR(md, "attr is key and cannot be modified");

        return LAI_STATUS_INVALID_PARAMETER;
    }

    lai_object_id_t linecard_id = LAI_NULL_OBJECT_ID;

    auto info = lai_metadata_get_object_type_info(meta_key.objecttype);

    if (!info->isnonobjectid)
    {
        linecard_id = linecardIdQuery(meta_key.objectkey.key.object_id);

        if (!m_oids.objectReferenceExists(linecard_id))
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist", linecard_id);
            return LAI_STATUS_INVALID_PARAMETER;
        }
    }

    linecard_id = meta_extract_linecard_id(meta_key, linecard_id);

    // if we set OID check if exists and if type is correct

    switch (md.attrvaluetype)
    {
        case LAI_ATTR_VALUE_TYPE_BOOL:
        case LAI_ATTR_VALUE_TYPE_UINT8:
        case LAI_ATTR_VALUE_TYPE_INT8:
        case LAI_ATTR_VALUE_TYPE_UINT16:
        case LAI_ATTR_VALUE_TYPE_INT16:
        case LAI_ATTR_VALUE_TYPE_UINT32:
        case LAI_ATTR_VALUE_TYPE_INT32:
        case LAI_ATTR_VALUE_TYPE_UINT64:
        case LAI_ATTR_VALUE_TYPE_INT64:
        case LAI_ATTR_VALUE_TYPE_DOUBLE:
        case LAI_ATTR_VALUE_TYPE_POINTER:
            // primitives
            break;

        case LAI_ATTR_VALUE_TYPE_CHARDATA:

            {
                size_t len = strnlen(value.chardata, sizeof(lai_attribute_value_t::chardata)/sizeof(char));

                // for some attributes, length can be zero

                for (size_t i = 0; i < len; ++i)
                {
                    char c = value.chardata[i];

                    if (c < 0x20 || c > 0x7e)
                    {
                        META_LOG_ERROR(md, "invalid character 0x%02x in chardata", c);

                        return LAI_STATUS_INVALID_PARAMETER;
                    }
                }

                break;
            }

        case LAI_ATTR_VALUE_TYPE_OBJECT_ID:

            {

                lai_status_t status = meta_generic_validation_objlist(md, linecard_id, 1, &value.oid);

                if (status != LAI_STATUS_SUCCESS)
                {
                    return status;
                }

                break;
            }

        case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:

            {
                lai_status_t status = meta_generic_validation_objlist(md, linecard_id, value.objlist.count, value.objlist.list);

                if (status != LAI_STATUS_SUCCESS)
                {
                    return status;
                }

                break;
            }

        case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
            VALIDATION_LIST(md, value.u8list);
            break;
        case LAI_ATTR_VALUE_TYPE_INT8_LIST:
            VALIDATION_LIST(md, value.s8list);
            break;
        case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
            VALIDATION_LIST(md, value.u16list);
            break;
        case LAI_ATTR_VALUE_TYPE_INT16_LIST:
            VALIDATION_LIST(md, value.s16list);
            break;
        case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
            VALIDATION_LIST(md, value.u32list);
            break;
        case LAI_ATTR_VALUE_TYPE_INT32_LIST:
            VALIDATION_LIST(md, value.s32list);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT32_RANGE:

            if (value.u32range.min > value.u32range.max)
            {
                META_LOG_ERROR(md, "invalid range %u .. %u", value.u32range.min, value.u32range.max);

                return LAI_STATUS_INVALID_PARAMETER;
            }

            break;

        case LAI_ATTR_VALUE_TYPE_INT32_RANGE:

            if (value.s32range.min > value.s32range.max)
            {
                META_LOG_ERROR(md, "invalid range %u .. %u", value.s32range.min, value.s32range.max);

                return LAI_STATUS_INVALID_PARAMETER;
            }

            break;

        default:

            META_LOG_THROW(md, "serialization type is not supported yet FIXME");
    }

    if (md.isenum)
    {
        int32_t val = value.s32;

        val = value.s32;

        if (!lai_metadata_is_allowed_enum_value(&md, val))
        {
            META_LOG_ERROR(md, "is enum, but value %d not found on allowed values list", val);

            return LAI_STATUS_INVALID_PARAMETER;
        }
    }

    if (md.isenumlist)
    {
        // we allow repeats on enum list
        if (value.s32list.count != 0 && value.s32list.list == NULL)
        {
            META_LOG_ERROR(md, "enum list is NULL");

            return LAI_STATUS_INVALID_PARAMETER;
        }

        for (uint32_t i = value.s32list.count; i < value.s32list.count; ++i)
        {
            int32_t s32 = value.s32list.list[i];

            if (!lai_metadata_is_allowed_enum_value(&md, s32))
            {
                SWSS_LOG_ERROR("is enum list, but value %d not found on allowed values list", s32);

                return LAI_STATUS_INVALID_PARAMETER;
            }
        }
    }

    if (md.isconditional)
    {
        // check if it was set on local DB
        // (this will not respect create_only with default)

        if (get_object_previous_attr(meta_key, md) == NULL)
        {
            META_LOG_WARN(md, "set for conditional, but not found in local db, object %s created on linecard ?",
                    lai_serialize_object_meta_key(meta_key).c_str());
        }
        else
        {
            META_LOG_DEBUG(md, "conditional attr found in local db");
        }

        META_LOG_DEBUG(md, "conditional attr found in local db");
    }

    // check if object on which we perform operation exists

    if (!m_laiObjectCollection.objectExists(meta_key))
    {
        META_LOG_ERROR(md, "object key %s doesn't exist",
                lai_serialize_object_meta_key(meta_key).c_str());

        return LAI_STATUS_INVALID_PARAMETER;
    }

    // object exists in DB so we can do "set" operation

    if (info->isnonobjectid)
    {
        SWSS_LOG_DEBUG("object key exists: %s",
                lai_serialize_object_meta_key(meta_key).c_str());
    }
    else
    {
        /*
         * Check if object we are calling SET is the same object type as the
         * type of SET function.
         */

        lai_object_id_t oid = meta_key.objectkey.key.object_id;

        lai_object_type_t object_type = objectTypeQuery(oid);

        if (object_type == LAI_NULL_OBJECT_ID)
        {
            META_LOG_ERROR(md, "oid 0x%" PRIx64 " is not valid, returned null object id", oid);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (object_type != meta_key.objecttype)
        {
            META_LOG_ERROR(md, "oid 0x%" PRIx64 " type %d is not accepted, expected object type %d", oid, object_type, meta_key.objecttype);

            return LAI_STATUS_INVALID_PARAMETER;
        }
    }

    return LAI_STATUS_SUCCESS;
}

lai_status_t Meta::meta_generic_validation_get(
        _In_ const lai_object_meta_key_t& meta_key,
        _In_ const uint32_t attr_count,
        _In_ lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (attr_count < 1)
    {
        SWSS_LOG_ERROR("expected at least 1 attribute when calling get, zero given");

        return LAI_STATUS_INVALID_PARAMETER;
    }

    if (attr_count > MAX_LIST_COUNT)
    {
        SWSS_LOG_ERROR("get attribute count %u > max list count %u", attr_count, MAX_LIST_COUNT);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    if (attr_list == NULL)
    {
        SWSS_LOG_ERROR("attribute list pointer is NULL");

        return LAI_STATUS_INVALID_PARAMETER;
    }

    SWSS_LOG_DEBUG("attr count = %u", attr_count);

    for (uint32_t i = 0; i < attr_count; ++i)
    {
        const lai_attribute_t* attr = &attr_list[i];

        auto mdp = lai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

        if (mdp == NULL)
        {
            SWSS_LOG_ERROR("unable to find attribute metadata %d:%d", meta_key.objecttype, attr->id);

            return LAI_STATUS_FAILURE;
        }

        const lai_attribute_value_t& value = attr->value;

        const lai_attr_metadata_t& md = *mdp;

        META_LOG_DEBUG(md, "(get)");

        if (LAI_HAS_FLAG_SET_ONLY(md.flags))
        {
            META_LOG_ERROR(md, "attr is write only and cannot be read");

            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (md.isconditional)
        {
            /*
             * XXX workaround
             *
             * TODO If object was created internally by switch (like bridge
             * port) then current db will not have previous value of this
             * attribute (like LAI_BRIDGE_PORT_ATTR_PORT_ID) or even other oid.
             * This can lead to inconsistency, that we queried one oid, and its
             * attribute also oid, and then did a "set" on that value, and now
             * reference is not decreased since previous oid was not snooped.
             *
             * TODO This concern all attributes not only conditionals
             *
             * If attribute is conditional, we need to check if condition is
             * met, if not then this attribute is not mandatory so we can
             * return fail in that case, for that we need all internal
             * linecard objects after create.
             */

            // check if it was set on local DB
            // (this will not respect create_only with default)
            if (get_object_previous_attr(meta_key, md) == NULL)
            {
                // XXX produces too much noise
                // META_LOG_WARN(md, "get for conditional, but not found in local db, object %s created on linecard ?",
                //          lai_serialize_object_meta_key(meta_key).c_str());
            }
            else
            {
                META_LOG_DEBUG(md, "conditional attr found in local db");
            }
        }

        /*
         * When GET api is performed, later on same methods serialize/deserialize
         * are used for create/set/get apis. User may not clear input attributes
         * buffer (since it is in/out for example for lists) and in case of
         * values that are validated like "enum" it will try to find best
         * match for enum, and if not found, it will print warning message.
         *
         * In this place we can clear user buffer, so when it will go to
         * serialize method it will pick first enum on the list.
         *
         * For primitive attributes we could just set entire attribute value to zero.
         */

        if (md.isenum)
        {
            attr_list[i].value.s32 = 0;
        }

        switch (md.attrvaluetype)
        {
            case LAI_ATTR_VALUE_TYPE_BOOL:
            case LAI_ATTR_VALUE_TYPE_CHARDATA:
            case LAI_ATTR_VALUE_TYPE_UINT8:
            case LAI_ATTR_VALUE_TYPE_INT8:
            case LAI_ATTR_VALUE_TYPE_UINT16:
            case LAI_ATTR_VALUE_TYPE_INT16:
            case LAI_ATTR_VALUE_TYPE_UINT32:
            case LAI_ATTR_VALUE_TYPE_INT32:
            case LAI_ATTR_VALUE_TYPE_UINT64:
            case LAI_ATTR_VALUE_TYPE_INT64:
            case LAI_ATTR_VALUE_TYPE_DOUBLE:
            case LAI_ATTR_VALUE_TYPE_POINTER:
                // primitives
                break;

            case LAI_ATTR_VALUE_TYPE_OBJECT_ID:
                break;

            case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:
                VALIDATION_LIST(md, value.objlist);
                break;

            case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
                VALIDATION_LIST(md, value.u8list);
                break;
            case LAI_ATTR_VALUE_TYPE_INT8_LIST:
                VALIDATION_LIST(md, value.s8list);
                break;
            case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
                VALIDATION_LIST(md, value.u16list);
                break;
            case LAI_ATTR_VALUE_TYPE_INT16_LIST:
                VALIDATION_LIST(md, value.s16list);
                break;
            case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
                VALIDATION_LIST(md, value.u32list);
                break;
            case LAI_ATTR_VALUE_TYPE_INT32_LIST:
                VALIDATION_LIST(md, value.s32list);
                break;

            case LAI_ATTR_VALUE_TYPE_UINT32_RANGE:
            case LAI_ATTR_VALUE_TYPE_INT32_RANGE:
                // primitives
                break;

            default:

                // acl capability will is more complex since is in/out we need to check stage

                META_LOG_THROW(md, "serialization type is not supported yet FIXME");
        }
    }

    if (!m_laiObjectCollection.objectExists(meta_key))
    {
        SWSS_LOG_ERROR("object key %s doesn't exist",
                lai_serialize_object_meta_key(meta_key).c_str());

        return LAI_STATUS_INVALID_PARAMETER;
    }

    auto info = lai_metadata_get_object_type_info(meta_key.objecttype);

    if (info->isnonobjectid)
    {
        SWSS_LOG_DEBUG("object key exists: %s",
                lai_serialize_object_meta_key(meta_key).c_str());
    }
    else
    {
        /*
         * Check if object we are calling GET is the same object type as the
         * type of GET function.
         */

        lai_object_id_t oid = meta_key.objectkey.key.object_id;

        lai_object_type_t object_type = objectTypeQuery(oid);

        if (object_type == LAI_NULL_OBJECT_ID)
        {
            SWSS_LOG_ERROR("oid 0x%" PRIx64 " is not valid, returned null object id", oid);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (object_type != meta_key.objecttype)
        {
            SWSS_LOG_ERROR("oid 0x%" PRIx64 " type %d is not accepted, expected object type %d", oid, object_type, meta_key.objecttype);

            return LAI_STATUS_INVALID_PARAMETER;
        }
    }

    // object exists in DB so we can do "get" operation

    return LAI_STATUS_SUCCESS;
}

void Meta::meta_generic_validation_post_get(
        _In_ const lai_object_meta_key_t& meta_key,
        _In_ lai_object_id_t linecard_id,
        _In_ const uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    linecard_id = meta_extract_linecard_id(meta_key, linecard_id);

    /*
     * TODO We should snoop attributes retrieved from linecard and put them to
     * local db if they don't exist since if attr is oid it may lead to
     * inconsistency when counting reference
     */

    for (uint32_t idx = 0; idx < attr_count; ++idx)
    {
        const lai_attribute_t* attr = &attr_list[idx];

        auto mdp = lai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

        const lai_attribute_value_t& value = attr->value;

        const lai_attr_metadata_t& md = *mdp;

        switch (md.attrvaluetype)
        {
            case LAI_ATTR_VALUE_TYPE_BOOL:
            case LAI_ATTR_VALUE_TYPE_CHARDATA:
            case LAI_ATTR_VALUE_TYPE_UINT8:
            case LAI_ATTR_VALUE_TYPE_INT8:
            case LAI_ATTR_VALUE_TYPE_UINT16:
            case LAI_ATTR_VALUE_TYPE_INT16:
            case LAI_ATTR_VALUE_TYPE_UINT32:
            case LAI_ATTR_VALUE_TYPE_INT32:
            case LAI_ATTR_VALUE_TYPE_UINT64:
            case LAI_ATTR_VALUE_TYPE_INT64:
            case LAI_ATTR_VALUE_TYPE_DOUBLE:
            case LAI_ATTR_VALUE_TYPE_POINTER:
                // primitives, ok
                break;

            case LAI_ATTR_VALUE_TYPE_OBJECT_ID:
                meta_generic_validation_post_get_objlist(meta_key, md, linecard_id, 1, &value.oid);
                break;

            case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:
                meta_generic_validation_post_get_objlist(meta_key, md, linecard_id, value.objlist.count, value.objlist.list);
                break;

            case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
                VALIDATION_LIST_GET(md, value.u8list);
                break;
            case LAI_ATTR_VALUE_TYPE_INT8_LIST:
                VALIDATION_LIST_GET(md, value.s8list);
                break;
            case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
                VALIDATION_LIST_GET(md, value.u16list);
                break;
            case LAI_ATTR_VALUE_TYPE_INT16_LIST:
                VALIDATION_LIST_GET(md, value.s16list);
                break;
            case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
                VALIDATION_LIST_GET(md, value.u32list);
                break;
            case LAI_ATTR_VALUE_TYPE_INT32_LIST:
                VALIDATION_LIST_GET(md, value.s32list);
                break;

            case LAI_ATTR_VALUE_TYPE_UINT32_RANGE:

                if (value.u32range.min > value.u32range.max)
                {
                    META_LOG_ERROR(md, "invalid range %u .. %u", value.u32range.min, value.u32range.max);
                }

                break;

            case LAI_ATTR_VALUE_TYPE_INT32_RANGE:

                if (value.s32range.min > value.s32range.max)
                {
                    META_LOG_ERROR(md, "invalid range %u .. %u", value.s32range.min, value.s32range.max);
                }

                break;

            default:

                META_LOG_THROW(md, "serialization type is not supported yet FIXME");
        }

        if (md.isenum)
        {
            int32_t val = value.s32;

            val = value.s32;

            if (!lai_metadata_is_allowed_enum_value(&md, val))
            {
                META_LOG_ERROR(md, "is enum, but value %d not found on allowed values list", val);
                continue;
            }
        }

        if (md.isenumlist)
        {
            if (value.s32list.list == NULL)
            {
                continue;
            }

            for (uint32_t i = value.s32list.count; i < value.s32list.count; ++i)
            {
                int32_t s32 = value.s32list.list[i];

                if (!lai_metadata_is_allowed_enum_value(&md, s32))
                {
                    META_LOG_ERROR(md, "is enum list, but value %d not found on allowed values list", s32);
                }
            }
        }
    }
}

lai_status_t Meta::meta_generic_validation_objlist(
        _In_ const lai_attr_metadata_t& md,
        _In_ lai_object_id_t linecard_id,
        _In_ uint32_t count,
        _In_ const lai_object_id_t* list)
{
    SWSS_LOG_ENTER();

    if (count > MAX_LIST_COUNT)
    {
        META_LOG_ERROR(md, "object list count %u > max list count %u", count, MAX_LIST_COUNT);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    if (list == NULL)
    {
        if (count == 0)
        {
            return LAI_STATUS_SUCCESS;
        }

        META_LOG_ERROR(md, "object list is null, but count is %u", count);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    /*
     * We need oids set and object type to check whether oids are not repeated
     * on list and whether all oids are same object type.
     */

    std::set<lai_object_id_t> oids;

    lai_object_type_t object_type = LAI_OBJECT_TYPE_NULL;

    for (uint32_t i = 0; i < count; ++i)
    {
        lai_object_id_t oid = list[i];

        if (oids.find(oid) != oids.end())
        {
            META_LOG_ERROR(md, "object on list [%u] oid 0x%" PRIx64 " is duplicated, but not allowed", i, oid);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (oid == LAI_NULL_OBJECT_ID)
        {
            if (md.allownullobjectid)
            {
                // ok, null object is allowed
                continue;
            }

            META_LOG_ERROR(md, "object on list [%u] is NULL, but not allowed", i);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        oids.insert(oid);

        lai_object_type_t ot = objectTypeQuery(oid);

        if (ot == LAI_NULL_OBJECT_ID)
        {
            META_LOG_ERROR(md, "object on list [%u] oid 0x%" PRIx64 " is not valid, returned null object id", i, oid);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (!lai_metadata_is_allowed_object_type(&md, ot))
        {
            META_LOG_ERROR(md, "object on list [%u] oid 0x%" PRIx64 " object type %d is not allowed on this attribute", i, oid, ot);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (!m_oids.objectReferenceExists(oid))
        {
            META_LOG_ERROR(md, "object on list [%u] oid 0x%" PRIx64 " object type %d does not exists in local DB", i, oid, ot);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (i > 1)
        {
            /*
             * Currently all objects on list must be the same type.
             */

            if (object_type != ot)
            {
                META_LOG_ERROR(md, "object list contain's mixed object types: %d vs %d, not allowed", object_type, ot);

                return LAI_STATUS_INVALID_PARAMETER;
            }
        }

        lai_object_id_t query_linecard_id = linecardIdQuery(oid);

        if (!m_oids.objectReferenceExists(query_linecard_id))
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist", query_linecard_id);
            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (query_linecard_id != linecard_id)
        {
            SWSS_LOG_ERROR("oid 0x%" PRIx64 " is from linecard 0x%" PRIx64 " but expected linecard 0x%" PRIx64 "", oid, query_linecard_id, linecard_id);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        object_type = ot;
    }

    return LAI_STATUS_SUCCESS;
}

lai_status_t Meta::meta_genetic_validation_list(
        _In_ const lai_attr_metadata_t& md,
        _In_ uint32_t count,
        _In_ const void* list)
{
    SWSS_LOG_ENTER();

    if (count > MAX_LIST_COUNT)
    {
        META_LOG_ERROR(md, "list count %u > max list count %u", count, MAX_LIST_COUNT);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    if (count == 0 && list != NULL)
    {
        META_LOG_ERROR(md, "when count is zero, list must be NULL");

        return LAI_STATUS_INVALID_PARAMETER;
    }

    if (list == NULL)
    {
        if (count == 0)
        {
            return LAI_STATUS_SUCCESS;
        }

        META_LOG_ERROR(md, "list is null, but count is %u", count);

        return LAI_STATUS_INVALID_PARAMETER;
    }

    return LAI_STATUS_SUCCESS;
}

lai_status_t Meta::meta_generic_validate_non_object_on_create(
        _In_ const lai_object_meta_key_t& meta_key,
        _In_ lai_object_id_t linecard_id)
{
    SWSS_LOG_ENTER();

    /*
     * Since non object id objects can contain several object id's inside
     * object id structure, we need to check whether they all belong to the
     * same switch (sine multiple linecards can be present and whether all those
     * objects are allowed respectively on their members.
     *
     * This check is required only on creation, since on set/get/remove we
     * check in object hash whether this object exists.
     */

    auto info = lai_metadata_get_object_type_info(meta_key.objecttype);

    if (!info->isnonobjectid)
    {
        return LAI_STATUS_SUCCESS;
    }

    /*
     * This will be most utilized for creating route entries.
     */

    for (size_t j = 0; j < info->structmemberscount; ++j)
    {
        const lai_struct_member_info_t *m = info->structmembers[j];

        if (m->membervaluetype != LAI_ATTR_VALUE_TYPE_OBJECT_ID)
        {
            continue;
        }

        lai_object_id_t oid = m->getoid(&meta_key);

        if (oid == LAI_NULL_OBJECT_ID)
        {
            SWSS_LOG_ERROR("oid on %s on struct member %s is NULL",
                    lai_serialize_object_type(meta_key.objecttype).c_str(),
                    m->membername);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (!m_oids.objectReferenceExists(oid))
        {
            SWSS_LOG_ERROR("object don't exist %s (%s)",
                    lai_serialize_object_id(oid).c_str(),
                    m->membername);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        lai_object_type_t ot = objectTypeQuery(oid);

        /*
         * No need for checking null here, since metadata don't allow
         * NULL in allowed objects list.
         */

        bool allowed = false;

        for (size_t k = 0 ; k < m->allowedobjecttypeslength; k++)
        {
            if (ot == m->allowedobjecttypes[k])
            {
                allowed = true;
                break;
            }
        }

        if (!allowed)
        {
            SWSS_LOG_ERROR("object id 0x%" PRIx64 " is %s, but it's not allowed on member %s",
                    oid, lai_serialize_object_type(ot).c_str(), m->membername);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        lai_object_id_t oid_linecard_id = linecardIdQuery(oid);

        if (!m_oids.objectReferenceExists(oid_linecard_id))
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist", oid_linecard_id);

            return LAI_STATUS_INVALID_PARAMETER;
        }

        if (linecard_id != oid_linecard_id)
        {
            SWSS_LOG_ERROR("oid 0x%" PRIx64 " is on linecard 0x%" PRIx64 " but required linecard is 0x%" PRIx64 "", oid, oid_linecard_id, linecard_id);

            return LAI_STATUS_INVALID_PARAMETER;
        }
    }

    return LAI_STATUS_SUCCESS;
}

lai_object_id_t Meta::meta_extract_linecard_id(
        _In_ const lai_object_meta_key_t& meta_key,
        _In_ lai_object_id_t linecard_id)
{
    SWSS_LOG_ENTER();

    /*
     * We assume here that objecttype in meta key is in valid range.
     */

    auto info = lai_metadata_get_object_type_info(meta_key.objecttype);

    if (info->isnonobjectid)
    {
        /*
         * Since object is non object id, we are sure via sanity check that
         * struct member contains linecard_id, we need to extract it here.
         *
         * NOTE: we could have this in metadata predefined for all non object ids.
         */

        for (size_t j = 0; j < info->structmemberscount; ++j)
        {
            const lai_struct_member_info_t *m = info->structmembers[j];

            if (m->membervaluetype != LAI_ATTR_VALUE_TYPE_OBJECT_ID)
            {
                continue;
            }

            for (size_t k = 0 ; k < m->allowedobjecttypeslength; k++)
            {
                lai_object_type_t ot = m->allowedobjecttypes[k];

                if (ot == LAI_OBJECT_TYPE_LINECARD)
                {
                    return  m->getoid(&meta_key);
                }
            }
        }

        SWSS_LOG_ERROR("unable to find linecard id inside non object id");

        return LAI_NULL_OBJECT_ID;
    }
    else
    {
        // NOTE: maybe we should extract linecard from oid?
        return linecard_id;
    }
}

std::shared_ptr<LaiAttrWrapper> Meta::get_object_previous_attr(
        _In_ const lai_object_meta_key_t& metaKey,
        _In_ const lai_attr_metadata_t& md)
{
    SWSS_LOG_ENTER();

    return m_laiObjectCollection.getObjectAttr(metaKey, md.attrid);
}

std::vector<const lai_attr_metadata_t*> Meta::get_attributes_metadata(
        _In_ lai_object_type_t objecttype)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_DEBUG("objecttype: %s", lai_serialize_object_type(objecttype).c_str());

    auto meta = lai_metadata_get_object_type_info(objecttype)->attrmetadata;

    std::vector<const lai_attr_metadata_t*> attrs;

    for (size_t index = 0; meta[index] != NULL; ++index)
    {
        attrs.push_back(meta[index]);
    }

    return attrs;
}

void Meta::meta_add_port_to_related_map(
        _In_ lai_object_id_t port_id,
        _In_ const lai_object_list_t& list)
{
    SWSS_LOG_ENTER();

    for (uint32_t i = 0; i < list.count; i++)
    {
        lai_object_id_t rel = list.list[i];

        if (rel == LAI_NULL_OBJECT_ID)
            SWSS_LOG_THROW("not expected NULL oid on the list");

        m_portRelatedSet.insert(port_id, rel);
    }
}

void Meta::meta_generic_validation_post_get_objlist(
        _In_ const lai_object_meta_key_t& meta_key,
        _In_ const lai_attr_metadata_t& md,
        _In_ lai_object_id_t linecard_id,
        _In_ uint32_t count,
        _In_ const lai_object_id_t* list)
{
    SWSS_LOG_ENTER();

    /*
     * TODO This is not good enough when object was created by linecard
     * internally and it have oid attributes, we need to insert them to local
     * db and increase reference count if object don't exist.
     *
     * Also this function maybe not best place to do it since it's not executed
     * when we doing get on acl field/action. But none of those are created
     * internally by linecard.
     *
     * TODO Similar stuff is with SET, when we will set oid object on existing
     * linecard object, but we will not have it's previous value.  We can check
     * whether default value is present and it's const NULL.
     */

    if (!LAI_HAS_FLAG_READ_ONLY(md.flags) && md.isoidattribute)
    {
        if (get_object_previous_attr(meta_key, md) == NULL)
        {
            // XXX produces too much noise
            // META_LOG_WARN(md, "post get, not in local db, FIX snoop!: %s",
            //          lai_serialize_object_meta_key(meta_key).c_str());
        }
    }

    if (count > MAX_LIST_COUNT)
    {
        META_LOG_ERROR(md, "returned get object list count %u > max list count %u", count, MAX_LIST_COUNT);
    }

    if (list == NULL)
    {
        // query was for length
        return;
    }

    std::set<lai_object_id_t> oids;

    for (uint32_t i = 0; i < count; ++i)
    {
        lai_object_id_t oid = list[i];

        if (oids.find(oid) != oids.end())
        {
            META_LOG_ERROR(md, "returned get object on list [%u] is duplicated, but not allowed", i);
            continue;
        }

        oids.insert(oid);

        if (oid == LAI_NULL_OBJECT_ID)
        {
            if (md.allownullobjectid)
            {
                // ok, null object is allowed
                continue;
            }

            META_LOG_ERROR(md, "returned get object on list [%u] is NULL, but not allowed", i);
            continue;
        }

        lai_object_type_t ot = objectTypeQuery(oid);

        if (ot == LAI_OBJECT_TYPE_NULL)
        {
            META_LOG_ERROR(md, "returned get object on list [%u] oid 0x%" PRIx64 " is not valid, returned null object type", i, oid);
            continue;
        }

        if (!lai_metadata_is_allowed_object_type(&md, ot))
        {
            META_LOG_ERROR(md, "returned get object on list [%u] oid 0x%" PRIx64 " object type %d is not allowed on this attribute", i, oid, ot);
        }

        if (!m_oids.objectReferenceExists(oid))
        {
            // NOTE: there may happen that user will request multiple object lists
            // and first list was retrieved ok, but second failed with overflow
            // then we may forget to snoop

            META_LOG_INFO(md, "returned get object on list [%u] oid 0x%" PRIx64 " object type %d does not exists in local DB (snoop)", i, oid, ot);

            lai_object_meta_key_t key = { .objecttype = ot, .objectkey = { .key = { .object_id = oid } } };

            m_oids.objectReferenceInsert(oid);

            if (!m_laiObjectCollection.objectExists(key))
            {
                m_laiObjectCollection.createObject(key);
            }
        }

        lai_object_id_t query_linecard_id = linecardIdQuery(oid);

        if (!m_oids.objectReferenceExists(query_linecard_id))
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist", query_linecard_id);
        }

        if (query_linecard_id != linecard_id)
        {
            SWSS_LOG_ERROR("oid 0x%" PRIx64 " is from linecard 0x%" PRIx64 " but expected linecard 0x%" PRIx64 "", oid, query_linecard_id, linecard_id);
        }
    }
}

void Meta::meta_generic_validation_post_create(
        _In_ const lai_object_meta_key_t& meta_key,
        _In_ lai_object_id_t linecard_id,
        _In_ const uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (m_laiObjectCollection.objectExists(meta_key))
    {
        if (m_warmBoot && meta_key.objecttype == LAI_OBJECT_TYPE_LINECARD)
        {
            SWSS_LOG_NOTICE("post linecard create after WARM BOOT");
        }
        else
        {
            SWSS_LOG_ERROR("object key %s already exists (vendor bug?)",
                    lai_serialize_object_meta_key(meta_key).c_str());

            // this may produce inconsistency
        }
    }

    if (m_warmBoot && meta_key.objecttype == LAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_NOTICE("skipping create linecard on WARM BOOT since it was already created");
    }
    else
    {
        m_laiObjectCollection.createObject(meta_key);
    }

    auto info = lai_metadata_get_object_type_info(meta_key.objecttype);

    if (info->isnonobjectid)
    {
        /*
         * Increase object reference count for all object ids in non object id
         * members.
         */

        for (size_t j = 0; j < info->structmemberscount; ++j)
        {
            const lai_struct_member_info_t *m = info->structmembers[j];

            if (m->membervaluetype != LAI_ATTR_VALUE_TYPE_OBJECT_ID)
            {
                continue;
            }

            m_oids.objectReferenceIncrement(m->getoid(&meta_key));
        }
    }
    else
    {
        /*
         * Check if object created was expected type as the type of CRATE
         * function.
         */

        do
        {
            lai_object_id_t oid = meta_key.objectkey.key.object_id;

            if (oid == LAI_NULL_OBJECT_ID)
            {
                SWSS_LOG_ERROR("created oid is null object id (vendor bug?)");
                break;
            }

            lai_object_type_t object_type = objectTypeQuery(oid);

            if (object_type == LAI_NULL_OBJECT_ID)
            {
                SWSS_LOG_ERROR("created oid 0x%" PRIx64 " is not valid object type after create (null) (vendor bug?)", oid);
                break;
            }

            if (object_type != meta_key.objecttype)
            {
                SWSS_LOG_ERROR("created oid 0x%" PRIx64 " type %s, expected %s (vendor bug?)",
                        oid,
                        lai_serialize_object_type(object_type).c_str(),
                        lai_serialize_object_type(meta_key.objecttype).c_str());
                break;
            }

            if (meta_key.objecttype != LAI_OBJECT_TYPE_LINECARD)
            {
                /*
                 * Check if created object linecard is the same as input linecard.
                 */

                lai_object_id_t query_linecard_id = linecardIdQuery(meta_key.objectkey.key.object_id);

                if (!m_oids.objectReferenceExists(query_linecard_id))
                {
                    SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist", query_linecard_id);
                    break;
                }

                if (linecard_id != query_linecard_id)
                {
                    SWSS_LOG_ERROR("created oid 0x%" PRIx64 " linecard id 0x%" PRIx64 " is different than requested 0x%" PRIx64 "",
                            oid, query_linecard_id, linecard_id);
                    break;
                }
            }

            if (m_warmBoot && meta_key.objecttype == LAI_OBJECT_TYPE_LINECARD)
            {
                SWSS_LOG_NOTICE("skip insert linecard reference insert in WARM_BOOT");
            }
            else
            {
                m_oids.objectReferenceInsert(oid);
            }

        } while (false);
    }

    if (m_warmBoot)
    {
        SWSS_LOG_NOTICE("m_warmBoot = false");

        m_warmBoot = false;
    }

    bool haskeys = false;

    for (uint32_t idx = 0; idx < attr_count; ++idx)
    {
        const lai_attribute_t* attr = &attr_list[idx];

        auto mdp = lai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

        const lai_attribute_value_t& value = attr->value;

        const lai_attr_metadata_t& md = *mdp;

        if (LAI_HAS_FLAG_KEY(md.flags))
        {
            haskeys = true;
            META_LOG_DEBUG(md, "attr is key");
        }

        // increase reference on object id types

        switch (md.attrvaluetype)
        {
            case LAI_ATTR_VALUE_TYPE_BOOL:
            case LAI_ATTR_VALUE_TYPE_CHARDATA:
            case LAI_ATTR_VALUE_TYPE_UINT8:
            case LAI_ATTR_VALUE_TYPE_INT8:
            case LAI_ATTR_VALUE_TYPE_UINT16:
            case LAI_ATTR_VALUE_TYPE_INT16:
            case LAI_ATTR_VALUE_TYPE_UINT32:
            case LAI_ATTR_VALUE_TYPE_INT32:
            case LAI_ATTR_VALUE_TYPE_UINT64:
            case LAI_ATTR_VALUE_TYPE_INT64:
            case LAI_ATTR_VALUE_TYPE_DOUBLE:
            case LAI_ATTR_VALUE_TYPE_POINTER:
                // primitives
                break;

            case LAI_ATTR_VALUE_TYPE_OBJECT_ID:
                m_oids.objectReferenceIncrement(value.oid);
                break;

            case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:
                m_oids.objectReferenceIncrement(value.objlist);
                break;

            case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
            case LAI_ATTR_VALUE_TYPE_INT8_LIST:
            case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
            case LAI_ATTR_VALUE_TYPE_INT16_LIST:
            case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
            case LAI_ATTR_VALUE_TYPE_INT32_LIST:
            case LAI_ATTR_VALUE_TYPE_UINT32_RANGE:
            case LAI_ATTR_VALUE_TYPE_INT32_RANGE:
                // no special action required
                break;

            default:

                META_LOG_THROW(md, "serialization type is not supported yet FIXME");
        }

        m_laiObjectCollection.setObjectAttr(meta_key, md, attr);
    }

    if (haskeys)
    {
        auto mKey = lai_serialize_object_meta_key(meta_key);

        auto attrKey = AttrKeyMap::constructKey(meta_key, attr_count, attr_list);

        m_attrKeys.insert(mKey, attrKey);
    }
}

void Meta::meta_generic_validation_post_set(
        _In_ const lai_object_meta_key_t& meta_key,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    auto mdp = lai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

    const lai_attribute_value_t& value = attr->value;

    const lai_attr_metadata_t& md = *mdp;

    /*
     * TODO We need to get previous value and make deal with references, check
     * if there is default value and if it's const.
     */

    if (!LAI_HAS_FLAG_READ_ONLY(md.flags) && md.isoidattribute)
    {
        if ((get_object_previous_attr(meta_key, md) == NULL) &&
                (md.defaultvaluetype != LAI_DEFAULT_VALUE_TYPE_CONST &&
                 md.defaultvaluetype != LAI_DEFAULT_VALUE_TYPE_EMPTY_LIST))
        {
            /*
             * If default value type will be internal then we should warn.
             */

            // XXX produces too much noise
            // META_LOG_WARN(md, "post set, not in local db, FIX snoop!: %s",
            //              lai_serialize_object_meta_key(meta_key).c_str());
        }
    }

    switch (md.attrvaluetype)
    {
        case LAI_ATTR_VALUE_TYPE_BOOL:
        case LAI_ATTR_VALUE_TYPE_CHARDATA:
        case LAI_ATTR_VALUE_TYPE_UINT8:
        case LAI_ATTR_VALUE_TYPE_INT8:
        case LAI_ATTR_VALUE_TYPE_UINT16:
        case LAI_ATTR_VALUE_TYPE_INT16:
        case LAI_ATTR_VALUE_TYPE_UINT32:
        case LAI_ATTR_VALUE_TYPE_INT32:
        case LAI_ATTR_VALUE_TYPE_UINT64:
        case LAI_ATTR_VALUE_TYPE_INT64:
        case LAI_ATTR_VALUE_TYPE_DOUBLE:
        case LAI_ATTR_VALUE_TYPE_POINTER:
            // primitives, ok
            break;

        case LAI_ATTR_VALUE_TYPE_OBJECT_ID:

            {
                auto prev = get_object_previous_attr(meta_key, md);

                if (prev != NULL)
                {
                    // decrease previous if it was set
                    m_oids.objectReferenceDecrement(prev->getLaiAttr()->value.oid);
                }

                m_oids.objectReferenceIncrement(value.oid);

                break;
            }

        case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:

            {
                auto prev = get_object_previous_attr(meta_key, md);

                if (prev != NULL)
                {
                    // decrease previous if it was set
                    m_oids.objectReferenceDecrement(prev->getLaiAttr()->value.objlist);
                }

                m_oids.objectReferenceIncrement(value.objlist);

                break;
            }

        case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
        case LAI_ATTR_VALUE_TYPE_INT8_LIST:
        case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
        case LAI_ATTR_VALUE_TYPE_INT16_LIST:
        case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
        case LAI_ATTR_VALUE_TYPE_INT32_LIST:
        case LAI_ATTR_VALUE_TYPE_UINT32_RANGE:
        case LAI_ATTR_VALUE_TYPE_INT32_RANGE:
            // no special action required
            break;

        default:
            META_LOG_THROW(md, "serialization type is not supported yet FIXME");
    }

    // only on create we need to increase entry object types members
    // save actual attributes and values to local db

    m_laiObjectCollection.setObjectAttr(meta_key, md, attr);
}

bool Meta::meta_unittests_get_and_erase_set_readonly_flag(
        _In_ const lai_attr_metadata_t& md)
{
    SWSS_LOG_ENTER();

    if (!m_unittestsEnabled)
    {
        // explicitly to not produce false alarms
        SWSS_LOG_NOTICE("unittests are not enabled");
        return false;
    }

    const auto &it = m_meta_unittests_set_readonly_set.find(md.attridname);

    if (it == m_meta_unittests_set_readonly_set.end())
    {
        SWSS_LOG_ERROR("%s is not present in readonly set", md.attridname);
        return false;
    }

    SWSS_LOG_INFO("%s is present in readonly set, erasing", md.attridname);

    m_meta_unittests_set_readonly_set.erase(it);

    return true;
}

void Meta::meta_unittests_enable(
        _In_ bool enable)
{
    SWSS_LOG_ENTER();

    m_unittestsEnabled = enable;
}

bool Meta::meta_unittests_enabled()
{
    SWSS_LOG_ENTER();

    return m_unittestsEnabled;
}

lai_status_t Meta::meta_unittests_allow_readonly_set_once(
        _In_ lai_object_type_t object_type,
        _In_ int32_t attr_id)
{
    SWSS_LOG_ENTER();

    if (!m_unittestsEnabled)
    {
        SWSS_LOG_NOTICE("unittests are not enabled");
        return LAI_STATUS_FAILURE;
    }

    auto *md = lai_metadata_get_attr_metadata(object_type, attr_id);

    if (md == NULL)
    {
        SWSS_LOG_ERROR("failed to get metadata for object type %d and attr id %d", object_type, attr_id);
        return LAI_STATUS_FAILURE;
    }

    if (!LAI_HAS_FLAG_READ_ONLY(md->flags))
    {
        SWSS_LOG_ERROR("attribute %s is not marked as READ_ONLY", md->attridname);
        return LAI_STATUS_FAILURE;
    }

    m_meta_unittests_set_readonly_set.insert(md->attridname);

    SWSS_LOG_INFO("enabling SET for readonly attribute: %s", md->attridname);

    return LAI_STATUS_SUCCESS;
}

void Meta::meta_lai_on_linecard_state_change(
        _In_ lai_object_id_t linecard_id,
        _In_ lai_oper_status_t linecard_oper_status)
{
    SWSS_LOG_ENTER();

    auto ot = objectTypeQuery(linecard_id);

    if (ot != LAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_WARN("linecard_id %s is of type %s, but expected LAI_OBJECT_TYPE_LINECARD",
                lai_serialize_object_id(linecard_id).c_str(),
                lai_serialize_object_type(ot).c_str());
    }

    lai_object_meta_key_t linecard_meta_key = { .objecttype = ot , .objectkey = { .key = { .object_id = linecard_id } } };

    if (!m_laiObjectCollection.objectExists(linecard_meta_key))
    {
        SWSS_LOG_ERROR("linecard_id %s don't exists in local database",
                lai_serialize_object_id(linecard_id).c_str());
    }

    // we should not snoop linecard_id, since linecard id should be created directly by user

    if (!lai_metadata_get_enum_value_name(
                &lai_metadata_enum_lai_oper_status_t,
                linecard_oper_status))
    {
        SWSS_LOG_WARN("linecard oper status value (%d) not found in lai_oper_status_t",
                linecard_oper_status);
    }
}

void Meta::meta_lai_on_linecard_shutdown_request(
        _In_ lai_object_id_t linecard_id)
{
    SWSS_LOG_ENTER();

    auto ot = objectTypeQuery(linecard_id);

    if (ot != LAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_WARN("linecard_id %s is of type %s, but expected LAI_OBJECT_TYPE_LINECARD",
                lai_serialize_object_id(linecard_id).c_str(),
                lai_serialize_object_type(ot).c_str());
    }

    lai_object_meta_key_t linecard_meta_key = { .objecttype = ot , .objectkey = { .key = { .object_id = linecard_id } } };

    if (!m_laiObjectCollection.objectExists(linecard_meta_key))
    {
        SWSS_LOG_ERROR("linecard_id %s don't exists in local database",
                lai_serialize_object_id(linecard_id).c_str());
    }

    // we should not snoop linecard_id, since linecard id should be created directly by user
}

int32_t Meta::getObjectReferenceCount(
        _In_ lai_object_id_t oid) const
{
    SWSS_LOG_ENTER();

    return m_oids.getObjectReferenceCount(oid);
}

bool Meta::objectExists(
        _In_ const lai_object_meta_key_t& mk) const
{
    SWSS_LOG_ENTER();

    return m_laiObjectCollection.objectExists(mk);
}

