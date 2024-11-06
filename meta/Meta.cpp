#include "Meta.h"

#include "swss/logger.h"
#include "otai_serialize.h"

#include "Globals.h"
#include "OtaiAttributeList.h"

#include <inttypes.h>

#include <set>

// TODO add validation for all oids belong to the same linecard

#define MAX_LIST_COUNT 0x1000

#define CHECK_STATUS_SUCCESS(s) { if ((s) != OTAI_STATUS_SUCCESS) return (s); }

#define VALIDATION_LIST(md,vlist) \
{\
    auto status1 = meta_genetic_validation_list(md,vlist.count,vlist.list);\
    if (status1 != OTAI_STATUS_SUCCESS)\
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

using namespace otaimeta;

Meta::Meta(
        _In_ std::shared_ptr<otairedis::OtaiInterface> impl):
    m_implementation(impl)
{
    SWSS_LOG_ENTER();
}

otai_status_t Meta::initialize(
        _In_ uint64_t flags,
        _In_ const otai_service_method_table_t *service_method_table)
{
    SWSS_LOG_ENTER();

    return m_implementation->initialize(flags, service_method_table);
}

otai_status_t Meta::uninitialize(void)
{
    SWSS_LOG_ENTER();

    return m_implementation->uninitialize();
}

otai_status_t Meta::linkCheck(_Out_ bool *up)
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
    m_otaiObjectCollection.clear();
    m_attrKeys.clear();

    SWSS_LOG_NOTICE("end");
}

bool Meta::isEmpty()
{
    SWSS_LOG_ENTER();

    return m_oids.getAllOids().empty()
        && m_attrKeys.getAllKeys().empty()
        && m_otaiObjectCollection.getAllKeys().empty();
}

otai_status_t Meta::remove(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id)
{
    SWSS_LOG_ENTER();

    otai_status_t status = meta_otai_validate_oid(object_type, &object_id, OTAI_NULL_OBJECT_ID, false);

    if (status != OTAI_STATUS_SUCCESS)
    {
        return status;
    }

    otai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = object_id } } };

    status = meta_generic_validation_remove(meta_key);

    if (status != OTAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = m_implementation->remove(object_type, object_id);

    if (status == OTAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("remove status: %s", otai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("remove status: %s", otai_serialize_status(status).c_str());
    }

    if (status == OTAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_remove(meta_key);
    }

    return status;
}

otai_status_t Meta::create(
        _In_ otai_object_type_t object_type,
        _Out_ otai_object_id_t* object_id,
        _In_ otai_object_id_t linecard_id,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    otai_status_t status = meta_otai_validate_oid(object_type, object_id, linecard_id, true);

    if (status != OTAI_STATUS_SUCCESS)
    {
        return status;
    }

    otai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = OTAI_NULL_OBJECT_ID } } };

    status = meta_generic_validation_create(meta_key, linecard_id, attr_count, attr_list);

    if (status != OTAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = m_implementation->create(object_type, object_id, linecard_id, attr_count, attr_list);

    if (status == OTAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("create status: %s", otai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("create status: %s", otai_serialize_status(status).c_str());
    }

    if (status == OTAI_STATUS_SUCCESS)
    {
        meta_key.objectkey.key.object_id = *object_id;

        if (meta_key.objecttype == OTAI_OBJECT_TYPE_LINECARD)
        {
            /*
             * We are creating linecard object, so linecard id must be the same as
             * just created object. We could use OTAI_NULL_OBJECT_ID in that
             * case and do special linecard inside post_create method.
             */

            linecard_id = *object_id;
        }

        meta_generic_validation_post_create(meta_key, linecard_id, attr_count, attr_list);
    }

    return status;
}

otai_status_t Meta::set(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    otai_status_t status = meta_otai_validate_oid(object_type, &object_id, OTAI_NULL_OBJECT_ID, false);

    if (status != OTAI_STATUS_SUCCESS)
    {
        return status;
    }

    otai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = object_id } } };

    status = meta_generic_validation_set(meta_key, attr);

    if (status != OTAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = m_implementation->set(object_type, object_id, attr);

    if (status == OTAI_STATUS_SUCCESS)
    {
        SWSS_LOG_DEBUG("set status: %s", otai_serialize_status(status).c_str());
    }
    else
    {
        SWSS_LOG_ERROR("set status: %s", otai_serialize_status(status).c_str());
    }

    if (status == OTAI_STATUS_SUCCESS)
    {
        meta_generic_validation_post_set(meta_key, attr);
    }

    return status;
}

otai_status_t Meta::get(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t attr_count,
        _Inout_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    otai_status_t status = meta_otai_validate_oid(object_type, &object_id, OTAI_NULL_OBJECT_ID, false);

    if (status != OTAI_STATUS_SUCCESS)
    {
        return status;
    }

    otai_object_meta_key_t meta_key = { .objecttype = object_type, .objectkey = { .key = { .object_id  = object_id } } };

    status = meta_generic_validation_get(meta_key, attr_count, attr_list);

    if (status != OTAI_STATUS_SUCCESS)
    {
        return status;
    }

    status = m_implementation->get(object_type, object_id, attr_count, attr_list);

    if (status == OTAI_STATUS_SUCCESS)
    {
        otai_object_id_t linecard_id = linecardIdQuery(object_id);

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
        return OTAI_STATUS_INVALID_PARAMETER; } }

#define PARAMETER_CHECK_OID_OBJECT_TYPE(param, OT) {                                        \
    otai_object_type_t _ot = objectTypeQuery(param);                                   \
    if (_ot != (OT)) {                                                                      \
        SWSS_LOG_ERROR("parameter " # param " %s object type is %s, but expected %s",       \
                otai_serialize_object_id(param).c_str(),                                     \
                otai_serialize_object_type(_ot).c_str(),                                     \
                otai_serialize_object_type(OT).c_str());                                     \
        return OTAI_STATUS_INVALID_PARAMETER; } }

#define PARAMETER_CHECK_OBJECT_TYPE_VALID(ot) {                                             \
    if (!otai_metadata_is_object_type_valid(ot)) {                                           \
        SWSS_LOG_ERROR("parameter " # ot " object type %d is invalid", (ot));               \
        return OTAI_STATUS_INVALID_PARAMETER; } }

#define PARAMETER_CHECK_POSITIVE(param) {                                                   \
    if ((param) <= 0) {                                                                     \
        SWSS_LOG_ERROR("parameter " #param " must be positive");                            \
        return OTAI_STATUS_INVALID_PARAMETER; } }

#define PARAMETER_CHECK_OID_EXISTS(oid, OT) {                                               \
    otai_object_meta_key_t _key = {                                                          \
        .objecttype = (OT), .objectkey = { .key = { .object_id = (oid) } } };               \
    if (!m_otaiObjectCollection.objectExists(_key)) {                                        \
        SWSS_LOG_ERROR("object %s don't exists", otai_serialize_object_id(oid).c_str()); } }

#define META_COUNTERS_COUNT_MSB (0x80000000)

otai_status_t Meta::meta_validate_stats(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids,
        _Out_ otai_stat_value_t *counters,
        _In_ otai_stats_mode_t mode)
{
    SWSS_LOG_ENTER();

    PARAMETER_CHECK_OBJECT_TYPE_VALID(object_type);
    PARAMETER_CHECK_OID_OBJECT_TYPE(object_id, object_type);
    PARAMETER_CHECK_OID_EXISTS(object_id, object_type);
    PARAMETER_CHECK_POSITIVE(number_of_counters);
    PARAMETER_CHECK_IF_NOT_NULL(counter_ids);
    PARAMETER_CHECK_IF_NOT_NULL(counters);

    otai_object_id_t linecard_id = linecardIdQuery(object_id);

    // checks also if object type is OID
    otai_status_t status = meta_otai_validate_oid(object_type, &object_id, linecard_id, false);

    CHECK_STATUS_SUCCESS(status);

    auto info = otai_metadata_get_object_type_info(object_type);

    PARAMETER_CHECK_IF_NOT_NULL(info);

    if (info->statenum == nullptr)
    {
        SWSS_LOG_ERROR("%s does not support stats", info->objecttypename);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    // check if all counter ids are in enum range

    for (uint32_t idx = 0; idx < number_of_counters; idx++)
    {
        if (otai_metadata_get_enum_value_name(info->statenum, counter_ids[idx]) == nullptr)
        {
            SWSS_LOG_ERROR("value %d is not in range on %s", counter_ids[idx], info->statenum->name);

            return OTAI_STATUS_INVALID_PARAMETER;
        }
    }

    // check mode

    if (otai_metadata_get_enum_value_name(&otai_metadata_enum_otai_stats_mode_t, mode) == nullptr)
    {
        SWSS_LOG_ERROR("mode value %d is not in range on %s", mode, otai_metadata_enum_otai_stats_mode_t.name);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    return OTAI_STATUS_SUCCESS;
}

otai_status_t Meta::getStats(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids,
        _Out_ otai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    auto status = meta_validate_stats(object_type, object_id, number_of_counters, counter_ids, counters, OTAI_STATS_MODE_READ);

    CHECK_STATUS_SUCCESS(status);

    status = m_implementation->getStats(object_type, object_id, number_of_counters, counter_ids, counters);

    // no post validation required

    return status;
}

otai_status_t Meta::getStatsExt(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids,
        _In_ otai_stats_mode_t mode,
        _Out_ otai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();

    auto status = meta_validate_stats(object_type, object_id, number_of_counters, counter_ids, counters, mode);

    CHECK_STATUS_SUCCESS(status);

    status = m_implementation->getStatsExt(object_type, object_id, number_of_counters, counter_ids, mode, counters);

    // no post validation required

    return status;
}

otai_status_t Meta::clearStats(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids)
{
    SWSS_LOG_ENTER();

    otai_stat_value_t counters;
    auto status = meta_validate_stats(object_type, object_id, number_of_counters, counter_ids, &counters, OTAI_STATS_MODE_READ);

    CHECK_STATUS_SUCCESS(status);

    status = m_implementation->clearStats(object_type, object_id, number_of_counters, counter_ids);

    // no post validation required

    return status;
}

otai_object_type_t Meta::objectTypeQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_implementation->objectTypeQuery(objectId);
}

otai_object_id_t Meta::linecardIdQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_implementation->linecardIdQuery(objectId);
}

otai_status_t Meta::logSet(
        _In_ otai_api_t api,
        _In_ otai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    // TODO check api and log level

    return m_implementation->logSet(api, log_level);
}

void Meta::clean_after_linecard_remove(
        _In_ otai_object_id_t linecardId)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("cleaning metadata for linecard: %s",
            otai_serialize_object_id(linecardId).c_str());

    if (objectTypeQuery(linecardId) != OTAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_THROW("oid %s is not LINECARD!",
                otai_serialize_object_id(linecardId).c_str());
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
        otai_object_meta_key_t mk;
        otai_deserialize_object_meta_key(key, mk);

        // we guarantee that linecard_id is first in the key structure so we can
        // use that as object_id as well

        if (linecardIdQuery(mk.objectkey.key.object_id) == linecardId)
        {
            m_attrKeys.eraseMetaKey(key);
        }
    }

    for (auto& mk: m_otaiObjectCollection.getAllKeys())
    {
        // we guarantee that linecard_id is first in the key structure so we can
        // use that as object_id as well

        if (linecardIdQuery(mk.objectkey.key.object_id) == linecardId)
        {
            m_otaiObjectCollection.removeObject(mk);
        }
    }

    SWSS_LOG_NOTICE("removed all objects related to linecard %s",
            otai_serialize_object_id(linecardId).c_str());
}

otai_status_t Meta::meta_generic_validation_remove(
        _In_ const otai_object_meta_key_t& meta_key)
{
    SWSS_LOG_ENTER();

    if (!m_otaiObjectCollection.objectExists(meta_key))
    {
        SWSS_LOG_ERROR("object key %s doesn't exist",
                otai_serialize_object_meta_key(meta_key).c_str());

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    auto info = otai_metadata_get_object_type_info(meta_key.objecttype);

    if (info->isnonobjectid)
    {
        // we don't keep reference of those since those are leafs
        return OTAI_STATUS_SUCCESS;
    }

    // for OID objects check oid value

    otai_object_id_t oid = meta_key.objectkey.key.object_id;

    if (oid == OTAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_ERROR("can't remove null object id");

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    otai_object_type_t object_type = objectTypeQuery(oid);

    if (object_type == OTAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_ERROR("oid 0x%" PRIx64 " is not valid, returned null object id", oid);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (object_type != meta_key.objecttype)
    {
        SWSS_LOG_ERROR("oid 0x%" PRIx64 " type %d is not accepted, expected object type %d", oid, object_type, meta_key.objecttype);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (!m_oids.objectReferenceExists(oid))
    {
        SWSS_LOG_ERROR("object 0x%" PRIx64 " reference doesn't exist", oid);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    int count = m_oids.getObjectReferenceCount(oid);

    if (count != 0)
    {
        if (object_type == OTAI_OBJECT_TYPE_LINECARD)
        {
            /*
             * We allow to remove linecard object even if there are ROUTE_ENTRY
             * created and referencing this linecard, since remove could be used
             * in WARM boot scenario.
             */

            SWSS_LOG_WARN("removing linecard object 0x%" PRIx64 " reference count is %d, removing all objects from meta DB", oid, count);

            return OTAI_STATUS_SUCCESS;
        }

        SWSS_LOG_ERROR("object 0x%" PRIx64 " reference count is %d, can't remove", oid, count);

        return OTAI_STATUS_OBJECT_IN_USE;
    }

    // should be safe to remove

    return OTAI_STATUS_SUCCESS;
}

otai_status_t Meta::meta_otai_validate_oid(
        _In_ otai_object_type_t object_type,
        _In_ const otai_object_id_t* object_id,
        _In_ otai_object_id_t linecard_id,
        _In_ bool bcreate)
{
    SWSS_LOG_ENTER();

    if (object_type <= OTAI_OBJECT_TYPE_NULL ||
            object_type >= OTAI_OBJECT_TYPE_EXTENSIONS_MAX)
    {
        SWSS_LOG_ERROR("invalid object type specified: %d, FIXME", object_type);
        return OTAI_STATUS_INVALID_PARAMETER;
    }

    const char* otname =  otai_metadata_get_enum_value_name(&otai_metadata_enum_otai_object_type_t, object_type);

    auto info = otai_metadata_get_object_type_info(object_type);

    if (info->isnonobjectid)
    {
        SWSS_LOG_THROW("invalid object type (%s) specified as generic, FIXME", otname);
    }

    SWSS_LOG_DEBUG("generic object type: %s", otname);

    if (object_id == NULL)
    {
        SWSS_LOG_ERROR("oid pointer is NULL");

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (bcreate)
    {
        return OTAI_STATUS_SUCCESS;
    }

    otai_object_id_t oid = *object_id;

    if (oid == OTAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_ERROR("oid is set to null object id on %s", otname);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    otai_object_type_t ot = objectTypeQuery(oid);

    if (ot == OTAI_OBJECT_TYPE_NULL)
    {
        SWSS_LOG_ERROR("%s oid 0x%" PRIx64 " is not valid object type, returned null object type", otname, oid);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    otai_object_type_t expected = object_type;

    if (ot != expected)
    {
        SWSS_LOG_ERROR("%s oid 0x%" PRIx64 " type %d is wrong type, expected object type %d", otname, oid, ot, expected);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    // check if object exists

    otai_object_meta_key_t meta_key_oid = { .objecttype = expected, .objectkey = { .key = { .object_id = oid } } };

    if (!m_otaiObjectCollection.objectExists(meta_key_oid))
    {
        SWSS_LOG_ERROR("object key %s doesn't exist",
                otai_serialize_object_meta_key(meta_key_oid).c_str());

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    return OTAI_STATUS_SUCCESS;
}

void Meta::meta_generic_validation_post_remove(
        _In_ const otai_object_meta_key_t& meta_key)
{
    SWSS_LOG_ENTER();

    if (meta_key.objecttype == OTAI_OBJECT_TYPE_LINECARD)
    {
        /*
         * If linecard object was removed then meta db was cleared and there are
         * no other attributes, no need for reference counting.
         */

        clean_after_linecard_remove(meta_key.objectkey.key.object_id);

        return;
    }

    // get all attributes that was set

    for (auto&it: m_otaiObjectCollection.getObject(meta_key)->getAttributes())
    {
        const otai_attribute_t* attr = it->getOtaiAttr();

        auto mdp = otai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

        const otai_attribute_value_t& value = attr->value;

        const otai_attr_metadata_t& md = *mdp;

        // decrease reference on object id types

        switch (md.attrvaluetype)
        {
            case OTAI_ATTR_VALUE_TYPE_BOOL:
            case OTAI_ATTR_VALUE_TYPE_CHARDATA:
            case OTAI_ATTR_VALUE_TYPE_UINT8:
            case OTAI_ATTR_VALUE_TYPE_INT8:
            case OTAI_ATTR_VALUE_TYPE_UINT16:
            case OTAI_ATTR_VALUE_TYPE_INT16:
            case OTAI_ATTR_VALUE_TYPE_UINT32:
            case OTAI_ATTR_VALUE_TYPE_INT32:
            case OTAI_ATTR_VALUE_TYPE_UINT64:
            case OTAI_ATTR_VALUE_TYPE_INT64:
            case OTAI_ATTR_VALUE_TYPE_DOUBLE:
            case OTAI_ATTR_VALUE_TYPE_POINTER:
                // primitives, ok
                break;

            case OTAI_ATTR_VALUE_TYPE_OBJECT_ID:
                m_oids.objectReferenceDecrement(value.oid);
                break;

            case OTAI_ATTR_VALUE_TYPE_OBJECT_LIST:
                m_oids.objectReferenceDecrement(value.objlist);
                break;

            case OTAI_ATTR_VALUE_TYPE_UINT8_LIST:
            case OTAI_ATTR_VALUE_TYPE_INT8_LIST:
            case OTAI_ATTR_VALUE_TYPE_UINT16_LIST:
            case OTAI_ATTR_VALUE_TYPE_INT16_LIST:
            case OTAI_ATTR_VALUE_TYPE_UINT32_LIST:
            case OTAI_ATTR_VALUE_TYPE_INT32_LIST:
            case OTAI_ATTR_VALUE_TYPE_UINT32_RANGE:
            case OTAI_ATTR_VALUE_TYPE_INT32_RANGE:
                // no special action required
                break;

            default:
                META_LOG_THROW(md, "serialization type is not supported yet FIXME");
        }
    }

    // we don't keep track of fdb, neighbor, route since
    // those are safe to remove any time (leafs)

    auto info = otai_metadata_get_object_type_info(meta_key.objecttype);

    if (info->isnonobjectid)
    {
        /*
         * Decrease object reference count for all object ids in non object id
         * members.
         */

        for (size_t j = 0; j < info->structmemberscount; ++j)
        {
            const otai_struct_member_info_t *m = info->structmembers[j];

            if (m->membervaluetype != OTAI_ATTR_VALUE_TYPE_OBJECT_ID)
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

    m_otaiObjectCollection.removeObject(meta_key);

    std::string metaKey = otai_serialize_object_meta_key(meta_key);

    m_attrKeys.eraseMetaKey(metaKey);

}

otai_status_t Meta::meta_generic_validation_create(
        _In_ const otai_object_meta_key_t& meta_key,
        _In_ otai_object_id_t linecard_id,
        _In_ const uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (attr_count > MAX_LIST_COUNT)
    {
        SWSS_LOG_ERROR("create attribute count %u > max list count %u", attr_count, MAX_LIST_COUNT);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (attr_count > 0 && attr_list == NULL)
    {
        SWSS_LOG_ERROR("attr count is %u but attribute list pointer is NULL", attr_count);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    bool linecardcreate = meta_key.objecttype == OTAI_OBJECT_TYPE_LINECARD;

    if (linecardcreate)
    {
        // we are creating linecard

        linecard_id = OTAI_NULL_OBJECT_ID;

        /*
         * Creating linecard can't have any object attributes set on it, OID
         * attributes must be applied on linecard using SET API.
         */

        for (uint32_t i = 0; i < attr_count; ++i)
        {
            auto meta = otai_metadata_get_attr_metadata(OTAI_OBJECT_TYPE_LINECARD, attr_list[i].id);

            if (meta == NULL)
            {
                SWSS_LOG_ERROR("attribute %d not found", attr_list[i].id);

                return OTAI_STATUS_INVALID_PARAMETER;
            }

            if (meta->isoidattribute)
            {
                SWSS_LOG_ERROR("%s is OID attribute, not allowed on create linecard", meta->attridname);

                return OTAI_STATUS_INVALID_PARAMETER;
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

        if (linecard_id == OTAI_NULL_OBJECT_ID)
        {
            SWSS_LOG_ERROR("linecard id is NULL for %s", otai_serialize_object_type(meta_key.objecttype).c_str());

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        otai_object_type_t sw_type = objectTypeQuery(linecard_id);

        if (sw_type != OTAI_OBJECT_TYPE_LINECARD)
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " type is %s, expected LINECARD", linecard_id, otai_serialize_object_type(sw_type).c_str());

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        // check if linecard exists

        otai_object_meta_key_t linecard_meta_key = { .objecttype = OTAI_OBJECT_TYPE_LINECARD, .objectkey = { .key = { .object_id = linecard_id } } };

        if (!m_otaiObjectCollection.objectExists(linecard_meta_key))
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist yet", linecard_id);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        if (!m_oids.objectReferenceExists(linecard_id))
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist yet", linecard_id);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        // ok
    }

    otai_status_t status = meta_generic_validate_non_object_on_create(meta_key, linecard_id);

    if (status != OTAI_STATUS_SUCCESS)
    {
        return status;
    }

    std::unordered_map<otai_attr_id_t, const otai_attribute_t*> attrs;

    SWSS_LOG_DEBUG("attr count = %u", attr_count);

    bool haskeys = false;

    // check each attribute separately
    for (uint32_t idx = 0; idx < attr_count; ++idx)
    {
        const otai_attribute_t* attr = &attr_list[idx];

        auto mdp = otai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

        if (mdp == NULL)
        {
            SWSS_LOG_ERROR("unable to find attribute metadata %d:%d", meta_key.objecttype, attr->id);

            return OTAI_STATUS_FAILURE;
        }

        const otai_attribute_value_t& value = attr->value;

        const otai_attr_metadata_t& md = *mdp;

        META_LOG_DEBUG(md, "(create)");

        if (attrs.find(attr->id) != attrs.end())
        {
            META_LOG_ERROR(md, "attribute id (%u) is defined on attr list multiple times", attr->id);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        attrs[attr->id] = attr;

        if (OTAI_HAS_FLAG_READ_ONLY(md.flags))
        {
            META_LOG_ERROR(md, "attr is read only and cannot be created");

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        if (OTAI_HAS_FLAG_KEY(md.flags))
        {
            haskeys = true;

            META_LOG_DEBUG(md, "attr is key");
        }

        // if we set OID check if exists and if type is correct
        // and it belongs to the same linecard id

        switch (md.attrvaluetype)
        {
            case OTAI_ATTR_VALUE_TYPE_BOOL:
            case OTAI_ATTR_VALUE_TYPE_UINT8:
            case OTAI_ATTR_VALUE_TYPE_INT8:
            case OTAI_ATTR_VALUE_TYPE_UINT16:
            case OTAI_ATTR_VALUE_TYPE_INT16:
            case OTAI_ATTR_VALUE_TYPE_UINT32:
            case OTAI_ATTR_VALUE_TYPE_INT32:
            case OTAI_ATTR_VALUE_TYPE_UINT64:
            case OTAI_ATTR_VALUE_TYPE_INT64:
            case OTAI_ATTR_VALUE_TYPE_DOUBLE:
            case OTAI_ATTR_VALUE_TYPE_POINTER:
            case OTAI_ATTR_VALUE_TYPE_CHARDATA:
                // primitives
                break;

            case OTAI_ATTR_VALUE_TYPE_OBJECT_ID:

                {
                    status = meta_generic_validation_objlist(md, linecard_id, 1, &value.oid);

                    if (status != OTAI_STATUS_SUCCESS)
                    {
                        return status;
                    }

                    break;
                }

            case OTAI_ATTR_VALUE_TYPE_OBJECT_LIST:

                {
                    status = meta_generic_validation_objlist(md, linecard_id, value.objlist.count, value.objlist.list);

                    if (status != OTAI_STATUS_SUCCESS)
                    {
                        return status;
                    }

                    break;
                }

            case OTAI_ATTR_VALUE_TYPE_UINT8_LIST:
                VALIDATION_LIST(md, value.u8list);
                break;
            case OTAI_ATTR_VALUE_TYPE_INT8_LIST:
                VALIDATION_LIST(md, value.s8list);
                break;
            case OTAI_ATTR_VALUE_TYPE_UINT16_LIST:
                VALIDATION_LIST(md, value.u16list);
                break;
            case OTAI_ATTR_VALUE_TYPE_INT16_LIST:
                VALIDATION_LIST(md, value.s16list);
                break;
            case OTAI_ATTR_VALUE_TYPE_UINT32_LIST:
                VALIDATION_LIST(md, value.u32list);
                break;
            case OTAI_ATTR_VALUE_TYPE_INT32_LIST:
                VALIDATION_LIST(md, value.s32list);
                break;

            case OTAI_ATTR_VALUE_TYPE_UINT32_RANGE:

                if (value.u32range.min > value.u32range.max)
                {
                    META_LOG_ERROR(md, "invalid range %u .. %u", value.u32range.min, value.u32range.max);

                    return OTAI_STATUS_INVALID_PARAMETER;
                }

                break;

            case OTAI_ATTR_VALUE_TYPE_INT32_RANGE:

                if (value.s32range.min > value.s32range.max)
                {
                    META_LOG_ERROR(md, "invalid range %u .. %u", value.s32range.min, value.s32range.max);

                    return OTAI_STATUS_INVALID_PARAMETER;
                }

                break;

            default:

                META_LOG_THROW(md, "serialization type is not supported yet FIXME");
        }

        if (md.isenum)
        {
            int32_t val = value.s32;

            val = value.s32;

            if (!otai_metadata_is_allowed_enum_value(&md, val))
            {
                META_LOG_ERROR(md, "is enum, but value %d not found on allowed values list", val);

                return OTAI_STATUS_INVALID_PARAMETER;
            }
        }

        if (md.isenumlist)
        {
            // we allow repeats on enum list
            if (value.s32list.count != 0 && value.s32list.list == NULL)
            {
                META_LOG_ERROR(md, "enum list is NULL");

                return OTAI_STATUS_INVALID_PARAMETER;
            }

            for (uint32_t i = value.s32list.count; i < value.s32list.count; ++i)
            {
                int32_t s32 = value.s32list.list[i];

                if (!otai_metadata_is_allowed_enum_value(&md, s32))
                {
                    META_LOG_ERROR(md, "is enum list, but value %d not found on allowed values list", s32);

                    return OTAI_STATUS_INVALID_PARAMETER;
                }
            }
        }

        // conditions are checked later on
    }

    // we are creating object, no need for check if exists (only key values needs to be checked)

    auto info = otai_metadata_get_object_type_info(meta_key.objecttype);

    if (info->isnonobjectid)
    {
        // just sanity check if object already exists

        if (m_otaiObjectCollection.objectExists(meta_key))
        {
            SWSS_LOG_ERROR("object key %s already exists",
                    otai_serialize_object_meta_key(meta_key).c_str());

            return OTAI_STATUS_ITEM_ALREADY_EXISTS;
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

        return OTAI_STATUS_FAILURE;
    }

    // check if all mandatory attributes were passed

    for (auto mdp: metadata)
    {
        const otai_attr_metadata_t& md = *mdp;

        if (!OTAI_HAS_FLAG_MANDATORY_ON_CREATE(md.flags))
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
             * OTAI_BUFFER_PROFILE_ATTR_POOL_ID attribute (see file otaibuffer.h).
             */

            META_LOG_ERROR(md, "attribute is mandatory but not passed in attr list");

            return OTAI_STATUS_MANDATORY_ATTRIBUTE_MISSING;
        }
    }

    // check if we need any conditional attributes
    for (auto mdp: metadata)
    {
        const otai_attr_metadata_t& md = *mdp;

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
            const auto& cmd = *otai_metadata_get_attr_metadata(meta_key.objecttype, c.attrid);

            const otai_attribute_value_t* cvalue = cmd.defaultvalue;

            const otai_attribute_t *cattr = otai_metadata_get_attr_by_id(c.attrid, attr_count, attr_list);

            if (cattr != NULL)
            {
                META_LOG_DEBUG(md, "condition attr %d was passed, using it's value", c.attrid);

                cvalue = &cattr->value;
            }

            if (cmd.attrvaluetype == OTAI_ATTR_VALUE_TYPE_BOOL)
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

                return OTAI_STATUS_INVALID_PARAMETER;
            }

            continue;
        }

        // is required, check if user passed it
        const auto &it = attrs.find(md.attrid);

        if (it == attrs.end())
        {
            META_LOG_ERROR(md, "attribute is conditional and is mandatory but not passed in attr list");

            return OTAI_STATUS_MANDATORY_ATTRIBUTE_MISSING;
        }
    }

    if (haskeys)
    {
        std::string key = AttrKeyMap::constructKey(meta_key, attr_count, attr_list);

        // since we didn't created oid yet, we don't know if attribute key exists, check all
        if (m_attrKeys.attrKeyExists(key))
        {
            SWSS_LOG_ERROR("attribute key %s already exists, can't create", key.c_str());

            return OTAI_STATUS_INVALID_PARAMETER;
        }
    }

    return OTAI_STATUS_SUCCESS;
}

otai_status_t Meta::meta_generic_validation_set(
        _In_ const otai_object_meta_key_t& meta_key,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    if (attr == NULL)
    {
        SWSS_LOG_ERROR("attribute pointer is NULL");

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    auto mdp = otai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

    if (mdp == NULL)
    {
        SWSS_LOG_ERROR("unable to find attribute metadata %d:%d", meta_key.objecttype, attr->id);

        return OTAI_STATUS_FAILURE;
    }

    const otai_attribute_value_t& value = attr->value;

    const otai_attr_metadata_t& md = *mdp;

    META_LOG_DEBUG(md, "(set)");

    if (OTAI_HAS_FLAG_READ_ONLY(md.flags))
    {
        META_LOG_ERROR(md, "attr is read only and cannot be modified");

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (OTAI_HAS_FLAG_CREATE_ONLY(md.flags))
    {
        META_LOG_ERROR(md, "attr is create only and cannot be modified");

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (OTAI_HAS_FLAG_KEY(md.flags))
    {
        META_LOG_ERROR(md, "attr is key and cannot be modified");

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    otai_object_id_t linecard_id = OTAI_NULL_OBJECT_ID;

    auto info = otai_metadata_get_object_type_info(meta_key.objecttype);

    if (!info->isnonobjectid)
    {
        linecard_id = linecardIdQuery(meta_key.objectkey.key.object_id);

        if (!m_oids.objectReferenceExists(linecard_id))
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist", linecard_id);
            return OTAI_STATUS_INVALID_PARAMETER;
        }
    }

    linecard_id = meta_extract_linecard_id(meta_key, linecard_id);

    // if we set OID check if exists and if type is correct

    switch (md.attrvaluetype)
    {
        case OTAI_ATTR_VALUE_TYPE_BOOL:
        case OTAI_ATTR_VALUE_TYPE_UINT8:
        case OTAI_ATTR_VALUE_TYPE_INT8:
        case OTAI_ATTR_VALUE_TYPE_UINT16:
        case OTAI_ATTR_VALUE_TYPE_INT16:
        case OTAI_ATTR_VALUE_TYPE_UINT32:
        case OTAI_ATTR_VALUE_TYPE_INT32:
        case OTAI_ATTR_VALUE_TYPE_UINT64:
        case OTAI_ATTR_VALUE_TYPE_INT64:
        case OTAI_ATTR_VALUE_TYPE_DOUBLE:
        case OTAI_ATTR_VALUE_TYPE_POINTER:
            // primitives
            break;

        case OTAI_ATTR_VALUE_TYPE_CHARDATA:

            {
                size_t len = strnlen(value.chardata, sizeof(otai_attribute_value_t::chardata)/sizeof(char));

                // for some attributes, length can be zero

                for (size_t i = 0; i < len; ++i)
                {
                    char c = value.chardata[i];

                    if (c < 0x20 || c > 0x7e)
                    {
                        META_LOG_ERROR(md, "invalid character 0x%02x in chardata", c);

                        return OTAI_STATUS_INVALID_PARAMETER;
                    }
                }

                break;
            }

        case OTAI_ATTR_VALUE_TYPE_OBJECT_ID:

            {

                otai_status_t status = meta_generic_validation_objlist(md, linecard_id, 1, &value.oid);

                if (status != OTAI_STATUS_SUCCESS)
                {
                    return status;
                }

                break;
            }

        case OTAI_ATTR_VALUE_TYPE_OBJECT_LIST:

            {
                otai_status_t status = meta_generic_validation_objlist(md, linecard_id, value.objlist.count, value.objlist.list);

                if (status != OTAI_STATUS_SUCCESS)
                {
                    return status;
                }

                break;
            }

        case OTAI_ATTR_VALUE_TYPE_UINT8_LIST:
            VALIDATION_LIST(md, value.u8list);
            break;
        case OTAI_ATTR_VALUE_TYPE_INT8_LIST:
            VALIDATION_LIST(md, value.s8list);
            break;
        case OTAI_ATTR_VALUE_TYPE_UINT16_LIST:
            VALIDATION_LIST(md, value.u16list);
            break;
        case OTAI_ATTR_VALUE_TYPE_INT16_LIST:
            VALIDATION_LIST(md, value.s16list);
            break;
        case OTAI_ATTR_VALUE_TYPE_UINT32_LIST:
            VALIDATION_LIST(md, value.u32list);
            break;
        case OTAI_ATTR_VALUE_TYPE_INT32_LIST:
            VALIDATION_LIST(md, value.s32list);
            break;

        case OTAI_ATTR_VALUE_TYPE_UINT32_RANGE:

            if (value.u32range.min > value.u32range.max)
            {
                META_LOG_ERROR(md, "invalid range %u .. %u", value.u32range.min, value.u32range.max);

                return OTAI_STATUS_INVALID_PARAMETER;
            }

            break;

        case OTAI_ATTR_VALUE_TYPE_INT32_RANGE:

            if (value.s32range.min > value.s32range.max)
            {
                META_LOG_ERROR(md, "invalid range %u .. %u", value.s32range.min, value.s32range.max);

                return OTAI_STATUS_INVALID_PARAMETER;
            }

            break;

        default:

            META_LOG_THROW(md, "serialization type is not supported yet FIXME");
    }

    if (md.isenum)
    {
        int32_t val = value.s32;

        val = value.s32;

        if (!otai_metadata_is_allowed_enum_value(&md, val))
        {
            META_LOG_ERROR(md, "is enum, but value %d not found on allowed values list", val);

            return OTAI_STATUS_INVALID_PARAMETER;
        }
    }

    if (md.isenumlist)
    {
        // we allow repeats on enum list
        if (value.s32list.count != 0 && value.s32list.list == NULL)
        {
            META_LOG_ERROR(md, "enum list is NULL");

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        for (uint32_t i = value.s32list.count; i < value.s32list.count; ++i)
        {
            int32_t s32 = value.s32list.list[i];

            if (!otai_metadata_is_allowed_enum_value(&md, s32))
            {
                SWSS_LOG_ERROR("is enum list, but value %d not found on allowed values list", s32);

                return OTAI_STATUS_INVALID_PARAMETER;
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
                    otai_serialize_object_meta_key(meta_key).c_str());
        }
        else
        {
            META_LOG_DEBUG(md, "conditional attr found in local db");
        }

        META_LOG_DEBUG(md, "conditional attr found in local db");
    }

    // check if object on which we perform operation exists

    if (!m_otaiObjectCollection.objectExists(meta_key))
    {
        META_LOG_ERROR(md, "object key %s doesn't exist",
                otai_serialize_object_meta_key(meta_key).c_str());

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    // object exists in DB so we can do "set" operation

    if (info->isnonobjectid)
    {
        SWSS_LOG_DEBUG("object key exists: %s",
                otai_serialize_object_meta_key(meta_key).c_str());
    }
    else
    {
        /*
         * Check if object we are calling SET is the same object type as the
         * type of SET function.
         */

        otai_object_id_t oid = meta_key.objectkey.key.object_id;

        otai_object_type_t object_type = objectTypeQuery(oid);

        if (object_type == OTAI_NULL_OBJECT_ID)
        {
            META_LOG_ERROR(md, "oid 0x%" PRIx64 " is not valid, returned null object id", oid);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        if (object_type != meta_key.objecttype)
        {
            META_LOG_ERROR(md, "oid 0x%" PRIx64 " type %d is not accepted, expected object type %d", oid, object_type, meta_key.objecttype);

            return OTAI_STATUS_INVALID_PARAMETER;
        }
    }

    return OTAI_STATUS_SUCCESS;
}

otai_status_t Meta::meta_generic_validation_get(
        _In_ const otai_object_meta_key_t& meta_key,
        _In_ const uint32_t attr_count,
        _In_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (attr_count < 1)
    {
        SWSS_LOG_ERROR("expected at least 1 attribute when calling get, zero given");

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (attr_count > MAX_LIST_COUNT)
    {
        SWSS_LOG_ERROR("get attribute count %u > max list count %u", attr_count, MAX_LIST_COUNT);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (attr_list == NULL)
    {
        SWSS_LOG_ERROR("attribute list pointer is NULL");

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    SWSS_LOG_DEBUG("attr count = %u", attr_count);

    for (uint32_t i = 0; i < attr_count; ++i)
    {
        const otai_attribute_t* attr = &attr_list[i];

        auto mdp = otai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

        if (mdp == NULL)
        {
            SWSS_LOG_ERROR("unable to find attribute metadata %d:%d", meta_key.objecttype, attr->id);

            return OTAI_STATUS_FAILURE;
        }

        const otai_attribute_value_t& value = attr->value;

        const otai_attr_metadata_t& md = *mdp;

        META_LOG_DEBUG(md, "(get)");

        if (OTAI_HAS_FLAG_SET_ONLY(md.flags))
        {
            META_LOG_ERROR(md, "attr is write only and cannot be read");

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        if (md.isconditional)
        {
            /*
             * XXX workaround
             *
             * TODO If object was created internally by switch (like bridge
             * port) then current db will not have previous value of this
             * attribute (like OTAI_BRIDGE_PORT_ATTR_PORT_ID) or even other oid.
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
                //          otai_serialize_object_meta_key(meta_key).c_str());
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
            case OTAI_ATTR_VALUE_TYPE_BOOL:
            case OTAI_ATTR_VALUE_TYPE_CHARDATA:
            case OTAI_ATTR_VALUE_TYPE_UINT8:
            case OTAI_ATTR_VALUE_TYPE_INT8:
            case OTAI_ATTR_VALUE_TYPE_UINT16:
            case OTAI_ATTR_VALUE_TYPE_INT16:
            case OTAI_ATTR_VALUE_TYPE_UINT32:
            case OTAI_ATTR_VALUE_TYPE_INT32:
            case OTAI_ATTR_VALUE_TYPE_UINT64:
            case OTAI_ATTR_VALUE_TYPE_INT64:
            case OTAI_ATTR_VALUE_TYPE_DOUBLE:
            case OTAI_ATTR_VALUE_TYPE_POINTER:
                // primitives
                break;

            case OTAI_ATTR_VALUE_TYPE_OBJECT_ID:
                break;

            case OTAI_ATTR_VALUE_TYPE_OBJECT_LIST:
                VALIDATION_LIST(md, value.objlist);
                break;

            case OTAI_ATTR_VALUE_TYPE_UINT8_LIST:
                VALIDATION_LIST(md, value.u8list);
                break;
            case OTAI_ATTR_VALUE_TYPE_INT8_LIST:
                VALIDATION_LIST(md, value.s8list);
                break;
            case OTAI_ATTR_VALUE_TYPE_UINT16_LIST:
                VALIDATION_LIST(md, value.u16list);
                break;
            case OTAI_ATTR_VALUE_TYPE_INT16_LIST:
                VALIDATION_LIST(md, value.s16list);
                break;
            case OTAI_ATTR_VALUE_TYPE_UINT32_LIST:
                VALIDATION_LIST(md, value.u32list);
                break;
            case OTAI_ATTR_VALUE_TYPE_INT32_LIST:
                VALIDATION_LIST(md, value.s32list);
                break;

            case OTAI_ATTR_VALUE_TYPE_UINT32_RANGE:
            case OTAI_ATTR_VALUE_TYPE_INT32_RANGE:
                // primitives
                break;

            default:

                // acl capability will is more complex since is in/out we need to check stage

                META_LOG_THROW(md, "serialization type is not supported yet FIXME");
        }
    }

    if (!m_otaiObjectCollection.objectExists(meta_key))
    {
        SWSS_LOG_ERROR("object key %s doesn't exist",
                otai_serialize_object_meta_key(meta_key).c_str());

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    auto info = otai_metadata_get_object_type_info(meta_key.objecttype);

    if (info->isnonobjectid)
    {
        SWSS_LOG_DEBUG("object key exists: %s",
                otai_serialize_object_meta_key(meta_key).c_str());
    }
    else
    {
        /*
         * Check if object we are calling GET is the same object type as the
         * type of GET function.
         */

        otai_object_id_t oid = meta_key.objectkey.key.object_id;

        otai_object_type_t object_type = objectTypeQuery(oid);

        if (object_type == OTAI_NULL_OBJECT_ID)
        {
            SWSS_LOG_ERROR("oid 0x%" PRIx64 " is not valid, returned null object id", oid);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        if (object_type != meta_key.objecttype)
        {
            SWSS_LOG_ERROR("oid 0x%" PRIx64 " type %d is not accepted, expected object type %d", oid, object_type, meta_key.objecttype);

            return OTAI_STATUS_INVALID_PARAMETER;
        }
    }

    // object exists in DB so we can do "get" operation

    return OTAI_STATUS_SUCCESS;
}

void Meta::meta_generic_validation_post_get(
        _In_ const otai_object_meta_key_t& meta_key,
        _In_ otai_object_id_t linecard_id,
        _In_ const uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
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
        const otai_attribute_t* attr = &attr_list[idx];

        auto mdp = otai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

        const otai_attribute_value_t& value = attr->value;

        const otai_attr_metadata_t& md = *mdp;

        switch (md.attrvaluetype)
        {
            case OTAI_ATTR_VALUE_TYPE_BOOL:
            case OTAI_ATTR_VALUE_TYPE_CHARDATA:
            case OTAI_ATTR_VALUE_TYPE_UINT8:
            case OTAI_ATTR_VALUE_TYPE_INT8:
            case OTAI_ATTR_VALUE_TYPE_UINT16:
            case OTAI_ATTR_VALUE_TYPE_INT16:
            case OTAI_ATTR_VALUE_TYPE_UINT32:
            case OTAI_ATTR_VALUE_TYPE_INT32:
            case OTAI_ATTR_VALUE_TYPE_UINT64:
            case OTAI_ATTR_VALUE_TYPE_INT64:
            case OTAI_ATTR_VALUE_TYPE_DOUBLE:
            case OTAI_ATTR_VALUE_TYPE_POINTER:
                // primitives, ok
                break;

            case OTAI_ATTR_VALUE_TYPE_OBJECT_ID:
                meta_generic_validation_post_get_objlist(meta_key, md, linecard_id, 1, &value.oid);
                break;

            case OTAI_ATTR_VALUE_TYPE_OBJECT_LIST:
                meta_generic_validation_post_get_objlist(meta_key, md, linecard_id, value.objlist.count, value.objlist.list);
                break;

            case OTAI_ATTR_VALUE_TYPE_UINT8_LIST:
                VALIDATION_LIST_GET(md, value.u8list);
                break;
            case OTAI_ATTR_VALUE_TYPE_INT8_LIST:
                VALIDATION_LIST_GET(md, value.s8list);
                break;
            case OTAI_ATTR_VALUE_TYPE_UINT16_LIST:
                VALIDATION_LIST_GET(md, value.u16list);
                break;
            case OTAI_ATTR_VALUE_TYPE_INT16_LIST:
                VALIDATION_LIST_GET(md, value.s16list);
                break;
            case OTAI_ATTR_VALUE_TYPE_UINT32_LIST:
                VALIDATION_LIST_GET(md, value.u32list);
                break;
            case OTAI_ATTR_VALUE_TYPE_INT32_LIST:
                VALIDATION_LIST_GET(md, value.s32list);
                break;

            case OTAI_ATTR_VALUE_TYPE_UINT32_RANGE:

                if (value.u32range.min > value.u32range.max)
                {
                    META_LOG_ERROR(md, "invalid range %u .. %u", value.u32range.min, value.u32range.max);
                }

                break;

            case OTAI_ATTR_VALUE_TYPE_INT32_RANGE:

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

            if (!otai_metadata_is_allowed_enum_value(&md, val))
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

                if (!otai_metadata_is_allowed_enum_value(&md, s32))
                {
                    META_LOG_ERROR(md, "is enum list, but value %d not found on allowed values list", s32);
                }
            }
        }
    }
}

otai_status_t Meta::meta_generic_validation_objlist(
        _In_ const otai_attr_metadata_t& md,
        _In_ otai_object_id_t linecard_id,
        _In_ uint32_t count,
        _In_ const otai_object_id_t* list)
{
    SWSS_LOG_ENTER();

    if (count > MAX_LIST_COUNT)
    {
        META_LOG_ERROR(md, "object list count %u > max list count %u", count, MAX_LIST_COUNT);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (list == NULL)
    {
        if (count == 0)
        {
            return OTAI_STATUS_SUCCESS;
        }

        META_LOG_ERROR(md, "object list is null, but count is %u", count);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    /*
     * We need oids set and object type to check whether oids are not repeated
     * on list and whether all oids are same object type.
     */

    std::set<otai_object_id_t> oids;

    otai_object_type_t object_type = OTAI_OBJECT_TYPE_NULL;

    for (uint32_t i = 0; i < count; ++i)
    {
        otai_object_id_t oid = list[i];

        if (oids.find(oid) != oids.end())
        {
            META_LOG_ERROR(md, "object on list [%u] oid 0x%" PRIx64 " is duplicated, but not allowed", i, oid);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        if (oid == OTAI_NULL_OBJECT_ID)
        {
            if (md.allownullobjectid)
            {
                // ok, null object is allowed
                continue;
            }

            META_LOG_ERROR(md, "object on list [%u] is NULL, but not allowed", i);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        oids.insert(oid);

        otai_object_type_t ot = objectTypeQuery(oid);

        if (ot == OTAI_NULL_OBJECT_ID)
        {
            META_LOG_ERROR(md, "object on list [%u] oid 0x%" PRIx64 " is not valid, returned null object id", i, oid);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        if (!otai_metadata_is_allowed_object_type(&md, ot))
        {
            META_LOG_ERROR(md, "object on list [%u] oid 0x%" PRIx64 " object type %d is not allowed on this attribute", i, oid, ot);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        if (!m_oids.objectReferenceExists(oid))
        {
            META_LOG_ERROR(md, "object on list [%u] oid 0x%" PRIx64 " object type %d does not exists in local DB", i, oid, ot);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        if (i > 1)
        {
            /*
             * Currently all objects on list must be the same type.
             */

            if (object_type != ot)
            {
                META_LOG_ERROR(md, "object list contain's mixed object types: %d vs %d, not allowed", object_type, ot);

                return OTAI_STATUS_INVALID_PARAMETER;
            }
        }

        otai_object_id_t query_linecard_id = linecardIdQuery(oid);

        if (!m_oids.objectReferenceExists(query_linecard_id))
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist", query_linecard_id);
            return OTAI_STATUS_INVALID_PARAMETER;
        }

        if (query_linecard_id != linecard_id)
        {
            SWSS_LOG_ERROR("oid 0x%" PRIx64 " is from linecard 0x%" PRIx64 " but expected linecard 0x%" PRIx64 "", oid, query_linecard_id, linecard_id);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        object_type = ot;
    }

    return OTAI_STATUS_SUCCESS;
}

otai_status_t Meta::meta_genetic_validation_list(
        _In_ const otai_attr_metadata_t& md,
        _In_ uint32_t count,
        _In_ const void* list)
{
    SWSS_LOG_ENTER();

    if (count > MAX_LIST_COUNT)
    {
        META_LOG_ERROR(md, "list count %u > max list count %u", count, MAX_LIST_COUNT);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (count == 0 && list != NULL)
    {
        META_LOG_ERROR(md, "when count is zero, list must be NULL");

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (list == NULL)
    {
        if (count == 0)
        {
            return OTAI_STATUS_SUCCESS;
        }

        META_LOG_ERROR(md, "list is null, but count is %u", count);

        return OTAI_STATUS_INVALID_PARAMETER;
    }

    return OTAI_STATUS_SUCCESS;
}

otai_status_t Meta::meta_generic_validate_non_object_on_create(
        _In_ const otai_object_meta_key_t& meta_key,
        _In_ otai_object_id_t linecard_id)
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

    auto info = otai_metadata_get_object_type_info(meta_key.objecttype);

    if (!info->isnonobjectid)
    {
        return OTAI_STATUS_SUCCESS;
    }

    /*
     * This will be most utilized for creating route entries.
     */

    for (size_t j = 0; j < info->structmemberscount; ++j)
    {
        const otai_struct_member_info_t *m = info->structmembers[j];

        if (m->membervaluetype != OTAI_ATTR_VALUE_TYPE_OBJECT_ID)
        {
            continue;
        }

        otai_object_id_t oid = m->getoid(&meta_key);

        if (oid == OTAI_NULL_OBJECT_ID)
        {
            SWSS_LOG_ERROR("oid on %s on struct member %s is NULL",
                    otai_serialize_object_type(meta_key.objecttype).c_str(),
                    m->membername);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        if (!m_oids.objectReferenceExists(oid))
        {
            SWSS_LOG_ERROR("object don't exist %s (%s)",
                    otai_serialize_object_id(oid).c_str(),
                    m->membername);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        otai_object_type_t ot = objectTypeQuery(oid);

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
                    oid, otai_serialize_object_type(ot).c_str(), m->membername);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        otai_object_id_t oid_linecard_id = linecardIdQuery(oid);

        if (!m_oids.objectReferenceExists(oid_linecard_id))
        {
            SWSS_LOG_ERROR("linecard id 0x%" PRIx64 " doesn't exist", oid_linecard_id);

            return OTAI_STATUS_INVALID_PARAMETER;
        }

        if (linecard_id != oid_linecard_id)
        {
            SWSS_LOG_ERROR("oid 0x%" PRIx64 " is on linecard 0x%" PRIx64 " but required linecard is 0x%" PRIx64 "", oid, oid_linecard_id, linecard_id);

            return OTAI_STATUS_INVALID_PARAMETER;
        }
    }

    return OTAI_STATUS_SUCCESS;
}

otai_object_id_t Meta::meta_extract_linecard_id(
        _In_ const otai_object_meta_key_t& meta_key,
        _In_ otai_object_id_t linecard_id)
{
    SWSS_LOG_ENTER();

    /*
     * We assume here that objecttype in meta key is in valid range.
     */

    auto info = otai_metadata_get_object_type_info(meta_key.objecttype);

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
            const otai_struct_member_info_t *m = info->structmembers[j];

            if (m->membervaluetype != OTAI_ATTR_VALUE_TYPE_OBJECT_ID)
            {
                continue;
            }

            for (size_t k = 0 ; k < m->allowedobjecttypeslength; k++)
            {
                otai_object_type_t ot = m->allowedobjecttypes[k];

                if (ot == OTAI_OBJECT_TYPE_LINECARD)
                {
                    return  m->getoid(&meta_key);
                }
            }
        }

        SWSS_LOG_ERROR("unable to find linecard id inside non object id");

        return OTAI_NULL_OBJECT_ID;
    }
    else
    {
        // NOTE: maybe we should extract linecard from oid?
        return linecard_id;
    }
}

std::shared_ptr<OtaiAttrWrapper> Meta::get_object_previous_attr(
        _In_ const otai_object_meta_key_t& metaKey,
        _In_ const otai_attr_metadata_t& md)
{
    SWSS_LOG_ENTER();

    return m_otaiObjectCollection.getObjectAttr(metaKey, md.attrid);
}

std::vector<const otai_attr_metadata_t*> Meta::get_attributes_metadata(
        _In_ otai_object_type_t objecttype)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_DEBUG("objecttype: %s", otai_serialize_object_type(objecttype).c_str());

    auto meta = otai_metadata_get_object_type_info(objecttype)->attrmetadata;

    std::vector<const otai_attr_metadata_t*> attrs;

    for (size_t index = 0; meta[index] != NULL; ++index)
    {
        attrs.push_back(meta[index]);
    }

    return attrs;
}

void Meta::meta_generic_validation_post_get_objlist(
        _In_ const otai_object_meta_key_t& meta_key,
        _In_ const otai_attr_metadata_t& md,
        _In_ otai_object_id_t linecard_id,
        _In_ uint32_t count,
        _In_ const otai_object_id_t* list)
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

    if (!OTAI_HAS_FLAG_READ_ONLY(md.flags) && md.isoidattribute)
    {
        if (get_object_previous_attr(meta_key, md) == NULL)
        {
            // XXX produces too much noise
            // META_LOG_WARN(md, "post get, not in local db, FIX snoop!: %s",
            //          otai_serialize_object_meta_key(meta_key).c_str());
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

    std::set<otai_object_id_t> oids;

    for (uint32_t i = 0; i < count; ++i)
    {
        otai_object_id_t oid = list[i];

        if (oids.find(oid) != oids.end())
        {
            META_LOG_ERROR(md, "returned get object on list [%u] is duplicated, but not allowed", i);
            continue;
        }

        oids.insert(oid);

        if (oid == OTAI_NULL_OBJECT_ID)
        {
            if (md.allownullobjectid)
            {
                // ok, null object is allowed
                continue;
            }

            META_LOG_ERROR(md, "returned get object on list [%u] is NULL, but not allowed", i);
            continue;
        }

        otai_object_type_t ot = objectTypeQuery(oid);

        if (ot == OTAI_OBJECT_TYPE_NULL)
        {
            META_LOG_ERROR(md, "returned get object on list [%u] oid 0x%" PRIx64 " is not valid, returned null object type", i, oid);
            continue;
        }

        if (!otai_metadata_is_allowed_object_type(&md, ot))
        {
            META_LOG_ERROR(md, "returned get object on list [%u] oid 0x%" PRIx64 " object type %d is not allowed on this attribute", i, oid, ot);
        }

        if (!m_oids.objectReferenceExists(oid))
        {
            // NOTE: there may happen that user will request multiple object lists
            // and first list was retrieved ok, but second failed with overflow
            // then we may forget to snoop

            META_LOG_INFO(md, "returned get object on list [%u] oid 0x%" PRIx64 " object type %d does not exists in local DB (snoop)", i, oid, ot);

            otai_object_meta_key_t key = { .objecttype = ot, .objectkey = { .key = { .object_id = oid } } };

            m_oids.objectReferenceInsert(oid);

            if (!m_otaiObjectCollection.objectExists(key))
            {
                m_otaiObjectCollection.createObject(key);
            }
        }

        otai_object_id_t query_linecard_id = linecardIdQuery(oid);

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
        _In_ const otai_object_meta_key_t& meta_key,
        _In_ otai_object_id_t linecard_id,
        _In_ const uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (m_otaiObjectCollection.objectExists(meta_key))
    {
        SWSS_LOG_ERROR("object key %s already exists (vendor bug?)",
                otai_serialize_object_meta_key(meta_key).c_str());
    }

    m_otaiObjectCollection.createObject(meta_key);

    auto info = otai_metadata_get_object_type_info(meta_key.objecttype);

    if (info->isnonobjectid)
    {
        /*
         * Increase object reference count for all object ids in non object id
         * members.
         */

        for (size_t j = 0; j < info->structmemberscount; ++j)
        {
            const otai_struct_member_info_t *m = info->structmembers[j];

            if (m->membervaluetype != OTAI_ATTR_VALUE_TYPE_OBJECT_ID)
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
            otai_object_id_t oid = meta_key.objectkey.key.object_id;

            if (oid == OTAI_NULL_OBJECT_ID)
            {
                SWSS_LOG_ERROR("created oid is null object id (vendor bug?)");
                break;
            }

            otai_object_type_t object_type = objectTypeQuery(oid);

            if (object_type == OTAI_NULL_OBJECT_ID)
            {
                SWSS_LOG_ERROR("created oid 0x%" PRIx64 " is not valid object type after create (null) (vendor bug?)", oid);
                break;
            }

            if (object_type != meta_key.objecttype)
            {
                SWSS_LOG_ERROR("created oid 0x%" PRIx64 " type %s, expected %s (vendor bug?)",
                        oid,
                        otai_serialize_object_type(object_type).c_str(),
                        otai_serialize_object_type(meta_key.objecttype).c_str());
                break;
            }

            if (meta_key.objecttype != OTAI_OBJECT_TYPE_LINECARD)
            {
                /*
                 * Check if created object linecard is the same as input linecard.
                 */

                otai_object_id_t query_linecard_id = linecardIdQuery(meta_key.objectkey.key.object_id);

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

            m_oids.objectReferenceInsert(oid);

        } while (false);
    }

    bool haskeys = false;

    for (uint32_t idx = 0; idx < attr_count; ++idx)
    {
        const otai_attribute_t* attr = &attr_list[idx];

        auto mdp = otai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

        const otai_attribute_value_t& value = attr->value;

        const otai_attr_metadata_t& md = *mdp;

        if (OTAI_HAS_FLAG_KEY(md.flags))
        {
            haskeys = true;
            META_LOG_DEBUG(md, "attr is key");
        }

        // increase reference on object id types

        switch (md.attrvaluetype)
        {
            case OTAI_ATTR_VALUE_TYPE_BOOL:
            case OTAI_ATTR_VALUE_TYPE_CHARDATA:
            case OTAI_ATTR_VALUE_TYPE_UINT8:
            case OTAI_ATTR_VALUE_TYPE_INT8:
            case OTAI_ATTR_VALUE_TYPE_UINT16:
            case OTAI_ATTR_VALUE_TYPE_INT16:
            case OTAI_ATTR_VALUE_TYPE_UINT32:
            case OTAI_ATTR_VALUE_TYPE_INT32:
            case OTAI_ATTR_VALUE_TYPE_UINT64:
            case OTAI_ATTR_VALUE_TYPE_INT64:
            case OTAI_ATTR_VALUE_TYPE_DOUBLE:
            case OTAI_ATTR_VALUE_TYPE_POINTER:
                // primitives
                break;

            case OTAI_ATTR_VALUE_TYPE_OBJECT_ID:
                m_oids.objectReferenceIncrement(value.oid);
                break;

            case OTAI_ATTR_VALUE_TYPE_OBJECT_LIST:
                m_oids.objectReferenceIncrement(value.objlist);
                break;

            case OTAI_ATTR_VALUE_TYPE_UINT8_LIST:
            case OTAI_ATTR_VALUE_TYPE_INT8_LIST:
            case OTAI_ATTR_VALUE_TYPE_UINT16_LIST:
            case OTAI_ATTR_VALUE_TYPE_INT16_LIST:
            case OTAI_ATTR_VALUE_TYPE_UINT32_LIST:
            case OTAI_ATTR_VALUE_TYPE_INT32_LIST:
            case OTAI_ATTR_VALUE_TYPE_UINT32_RANGE:
            case OTAI_ATTR_VALUE_TYPE_INT32_RANGE:
                // no special action required
                break;

            default:

                META_LOG_THROW(md, "serialization type is not supported yet FIXME");
        }

        m_otaiObjectCollection.setObjectAttr(meta_key, md, attr);
    }

    if (haskeys)
    {
        auto mKey = otai_serialize_object_meta_key(meta_key);

        auto attrKey = AttrKeyMap::constructKey(meta_key, attr_count, attr_list);

        m_attrKeys.insert(mKey, attrKey);
    }
}

void Meta::meta_generic_validation_post_set(
        _In_ const otai_object_meta_key_t& meta_key,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    auto mdp = otai_metadata_get_attr_metadata(meta_key.objecttype, attr->id);

    const otai_attribute_value_t& value = attr->value;

    const otai_attr_metadata_t& md = *mdp;

    /*
     * TODO We need to get previous value and make deal with references, check
     * if there is default value and if it's const.
     */

    if (!OTAI_HAS_FLAG_READ_ONLY(md.flags) && md.isoidattribute)
    {
        if ((get_object_previous_attr(meta_key, md) == NULL) &&
                (md.defaultvaluetype != OTAI_DEFAULT_VALUE_TYPE_CONST &&
                 md.defaultvaluetype != OTAI_DEFAULT_VALUE_TYPE_EMPTY_LIST))
        {
            /*
             * If default value type will be internal then we should warn.
             */

            // XXX produces too much noise
            // META_LOG_WARN(md, "post set, not in local db, FIX snoop!: %s",
            //              otai_serialize_object_meta_key(meta_key).c_str());
        }
    }

    switch (md.attrvaluetype)
    {
        case OTAI_ATTR_VALUE_TYPE_BOOL:
        case OTAI_ATTR_VALUE_TYPE_CHARDATA:
        case OTAI_ATTR_VALUE_TYPE_UINT8:
        case OTAI_ATTR_VALUE_TYPE_INT8:
        case OTAI_ATTR_VALUE_TYPE_UINT16:
        case OTAI_ATTR_VALUE_TYPE_INT16:
        case OTAI_ATTR_VALUE_TYPE_UINT32:
        case OTAI_ATTR_VALUE_TYPE_INT32:
        case OTAI_ATTR_VALUE_TYPE_UINT64:
        case OTAI_ATTR_VALUE_TYPE_INT64:
        case OTAI_ATTR_VALUE_TYPE_DOUBLE:
        case OTAI_ATTR_VALUE_TYPE_POINTER:
            // primitives, ok
            break;

        case OTAI_ATTR_VALUE_TYPE_OBJECT_ID:

            {
                auto prev = get_object_previous_attr(meta_key, md);

                if (prev != NULL)
                {
                    // decrease previous if it was set
                    m_oids.objectReferenceDecrement(prev->getOtaiAttr()->value.oid);
                }

                m_oids.objectReferenceIncrement(value.oid);

                break;
            }

        case OTAI_ATTR_VALUE_TYPE_OBJECT_LIST:

            {
                auto prev = get_object_previous_attr(meta_key, md);

                if (prev != NULL)
                {
                    // decrease previous if it was set
                    m_oids.objectReferenceDecrement(prev->getOtaiAttr()->value.objlist);
                }

                m_oids.objectReferenceIncrement(value.objlist);

                break;
            }

        case OTAI_ATTR_VALUE_TYPE_UINT8_LIST:
        case OTAI_ATTR_VALUE_TYPE_INT8_LIST:
        case OTAI_ATTR_VALUE_TYPE_UINT16_LIST:
        case OTAI_ATTR_VALUE_TYPE_INT16_LIST:
        case OTAI_ATTR_VALUE_TYPE_UINT32_LIST:
        case OTAI_ATTR_VALUE_TYPE_INT32_LIST:
        case OTAI_ATTR_VALUE_TYPE_UINT32_RANGE:
        case OTAI_ATTR_VALUE_TYPE_INT32_RANGE:
            // no special action required
            break;

        default:
            META_LOG_THROW(md, "serialization type is not supported yet FIXME");
    }

    // only on create we need to increase entry object types members
    // save actual attributes and values to local db

    m_otaiObjectCollection.setObjectAttr(meta_key, md, attr);
}

void Meta::meta_otai_on_linecard_state_change(
        _In_ otai_object_id_t linecard_id,
        _In_ otai_oper_status_t linecard_oper_status)
{
    SWSS_LOG_ENTER();

    auto ot = objectTypeQuery(linecard_id);

    if (ot != OTAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_WARN("linecard_id %s is of type %s, but expected OTAI_OBJECT_TYPE_LINECARD",
                otai_serialize_object_id(linecard_id).c_str(),
                otai_serialize_object_type(ot).c_str());
    }

    otai_object_meta_key_t linecard_meta_key = { .objecttype = ot , .objectkey = { .key = { .object_id = linecard_id } } };

    if (!m_otaiObjectCollection.objectExists(linecard_meta_key))
    {
        SWSS_LOG_ERROR("linecard_id %s don't exists in local database",
                otai_serialize_object_id(linecard_id).c_str());
    }

    // we should not snoop linecard_id, since linecard id should be created directly by user

    if (!otai_metadata_get_enum_value_name(
                &otai_metadata_enum_otai_oper_status_t,
                linecard_oper_status))
    {
        SWSS_LOG_WARN("linecard oper status value (%d) not found in otai_oper_status_t",
                linecard_oper_status);
    }
}

void Meta::meta_otai_on_linecard_shutdown_request(
        _In_ otai_object_id_t linecard_id)
{
    SWSS_LOG_ENTER();

    auto ot = objectTypeQuery(linecard_id);

    if (ot != OTAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_WARN("linecard_id %s is of type %s, but expected OTAI_OBJECT_TYPE_LINECARD",
                otai_serialize_object_id(linecard_id).c_str(),
                otai_serialize_object_type(ot).c_str());
    }

    otai_object_meta_key_t linecard_meta_key = { .objecttype = ot , .objectkey = { .key = { .object_id = linecard_id } } };

    if (!m_otaiObjectCollection.objectExists(linecard_meta_key))
    {
        SWSS_LOG_ERROR("linecard_id %s don't exists in local database",
                otai_serialize_object_id(linecard_id).c_str());
    }

    // we should not snoop linecard_id, since linecard id should be created directly by user
}

int32_t Meta::getObjectReferenceCount(
        _In_ otai_object_id_t oid) const
{
    SWSS_LOG_ENTER();

    return m_oids.getObjectReferenceCount(oid);
}

bool Meta::objectExists(
        _In_ const otai_object_meta_key_t& mk) const
{
    SWSS_LOG_ENTER();

    return m_otaiObjectCollection.objectExists(mk);
}

