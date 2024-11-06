#include "Meta.h"

#include "swss/logger.h"
#include "otai_serialize.h"

#include "Globals.h"

#include <inttypes.h>

#include <set>
#include <unordered_map>

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

otai_status_t Meta::meta_generic_validation_remove(
        _In_ const otai_object_meta_key_t& meta_key)
{
    SWSS_LOG_ENTER();

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

    return OTAI_STATUS_SUCCESS;
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

        // ok
    }

    otai_status_t status = meta_generic_validate_non_object_on_create(meta_key, linecard_id);

    if (status != OTAI_STATUS_SUCCESS)
    {
        return status;
    }

    std::unordered_map<otai_attr_id_t, const otai_attribute_t*> attrs;

    SWSS_LOG_DEBUG("attr count = %u", attr_count);

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

        otai_object_id_t query_linecard_id = linecardIdQuery(oid);
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

    auto info = otai_metadata_get_object_type_info(meta_key.objecttype);

    if (!info->isnonobjectid)
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

                if (linecard_id != query_linecard_id)
                {
                    SWSS_LOG_ERROR("created oid 0x%" PRIx64 " linecard id 0x%" PRIx64 " is different than requested 0x%" PRIx64 "",
                            oid, query_linecard_id, linecard_id);
                    break;
                }
            }
        } while (false);
    }
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

    // we should not snoop linecard_id, since linecard id should be created directly by user
}


