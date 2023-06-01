#include "lai_serialize.h"
#include "swss/tokenize.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include "swss/json.hpp"
#pragma GCC diagnostic pop

#include <inttypes.h>
#include <vector>
#include <climits>

#include <arpa/inet.h>
#include <errno.h>

using json = nlohmann::json;

#define ACCURATE (1.0 * 1e-18)

template<class T, typename U>
T* lai_alloc_n_of_ptr_type(U count, T*)
{
    SWSS_LOG_ENTER();

    return new T[count];
}

template<typename T, typename U>
void lai_alloc_list(
        _In_ T count,
        _In_ U &element)
{
    SWSS_LOG_ENTER();

    element.count = count;
    element.list = lai_alloc_n_of_ptr_type(count, element.list);
}

template<typename T>
void lai_free_list(
        _In_ T &element)
{
    SWSS_LOG_ENTER();

    delete[] element.list;
    element.list = NULL;
}

template<typename T>
static void transfer_primitive(
        _In_ const T &src_element,
        _In_ T &dst_element)
{
    SWSS_LOG_ENTER();

    const unsigned char* src_mem = reinterpret_cast<const unsigned char*>(&src_element);
    unsigned char* dst_mem = reinterpret_cast<unsigned char*>(&dst_element);

    memcpy(dst_mem, src_mem, sizeof(T));
}

template<typename T>
static lai_status_t transfer_list(
        _In_ const T &src_element,
        _In_ T &dst_element,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    if (countOnly || dst_element.count == 0)
    {
        transfer_primitive(src_element.count, dst_element.count);
        return LAI_STATUS_SUCCESS;
    }

    if (dst_element.list == NULL)
    {
        SWSS_LOG_ERROR("destination list is null, unable to transfer elements");

        return LAI_STATUS_FAILURE;
    }

    if (dst_element.count >= src_element.count)
    {
        if (src_element.list == NULL && src_element.count > 0)
        {
            SWSS_LOG_THROW("source list is NULL when count is %u, wrong db insert?", src_element.count);
        }

        transfer_primitive(src_element.count, dst_element.count);

        for (size_t i = 0; i < src_element.count; i++)
        {
            transfer_primitive(src_element.list[i], dst_element.list[i]);
        }

        return LAI_STATUS_SUCCESS;
    }

    // input buffer is too small to get all list elements, so return count only
    transfer_primitive(src_element.count, dst_element.count);

    return LAI_STATUS_BUFFER_OVERFLOW;
}

#define RETURN_ON_ERROR(x)\
{\
    lai_status_t s = (x);\
    if (s != LAI_STATUS_SUCCESS)\
        return s;\
}

lai_status_t transfer_attribute(
        _In_ lai_attr_value_type_t serialization_type,
        _In_ const lai_attribute_t &src_attr,
        _In_ lai_attribute_t &dst_attr,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    switch (serialization_type)
    {
        case LAI_ATTR_VALUE_TYPE_BOOL:
            transfer_primitive(src_attr.value.booldata, dst_attr.value.booldata);
            break;

        case LAI_ATTR_VALUE_TYPE_CHARDATA:
            transfer_primitive(src_attr.value.chardata, dst_attr.value.chardata);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT8:
            transfer_primitive(src_attr.value.u8, dst_attr.value.u8);
            break;

        case LAI_ATTR_VALUE_TYPE_INT8:
            transfer_primitive(src_attr.value.s8, dst_attr.value.s8);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT16:
            transfer_primitive(src_attr.value.u16, dst_attr.value.u16);
            break;

        case LAI_ATTR_VALUE_TYPE_INT16:
            transfer_primitive(src_attr.value.s16, dst_attr.value.s16);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT32:
            transfer_primitive(src_attr.value.u32, dst_attr.value.u32);
            break;

        case LAI_ATTR_VALUE_TYPE_INT32:
            transfer_primitive(src_attr.value.s32, dst_attr.value.s32);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT64:
            transfer_primitive(src_attr.value.u64, dst_attr.value.u64);
            break;

        case LAI_ATTR_VALUE_TYPE_INT64:
            transfer_primitive(src_attr.value.s64, dst_attr.value.s64);
            break;

        case LAI_ATTR_VALUE_TYPE_DOUBLE:
            transfer_primitive(src_attr.value.d64, dst_attr.value.d64);
            break;

        case LAI_ATTR_VALUE_TYPE_POINTER:
            transfer_primitive(src_attr.value.ptr, dst_attr.value.ptr);
            break;

        case LAI_ATTR_VALUE_TYPE_OBJECT_ID:
            transfer_primitive(src_attr.value.oid, dst_attr.value.oid);
            break;

        case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.objlist, dst_attr.value.objlist, countOnly));
            break;

        case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.u8list, dst_attr.value.u8list, countOnly));
            break;

        case LAI_ATTR_VALUE_TYPE_INT8_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.s8list, dst_attr.value.s8list, countOnly));
            break;

        case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.u16list, dst_attr.value.u16list, countOnly));
            break;

        case LAI_ATTR_VALUE_TYPE_INT16_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.s16list, dst_attr.value.s16list, countOnly));
            break;

        case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.u32list, dst_attr.value.u32list, countOnly));
            break;

        case LAI_ATTR_VALUE_TYPE_INT32_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.s32list, dst_attr.value.s32list, countOnly));
            break;

        case LAI_ATTR_VALUE_TYPE_UINT32_RANGE:
            transfer_primitive(src_attr.value.u32range, dst_attr.value.u32range);
            break;

        case LAI_ATTR_VALUE_TYPE_INT32_RANGE:
            transfer_primitive(src_attr.value.s32range, dst_attr.value.s32range);
            break;

        default:
            return LAI_STATUS_NOT_IMPLEMENTED;
    }

    return LAI_STATUS_SUCCESS;
}

lai_status_t transfer_attributes(
        _In_ lai_object_type_t object_type,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *src_attr_list,
        _In_ lai_attribute_t *dst_attr_list,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    for (uint32_t i = 0; i < attr_count; i++)
    {
        const lai_attribute_t &src_attr = src_attr_list[i];
        lai_attribute_t &dst_attr = dst_attr_list[i];

        auto meta = lai_metadata_get_attr_metadata(object_type, src_attr.id);

        if (src_attr.id != dst_attr.id)
        {
            SWSS_LOG_THROW("src (%d) vs dst (%d) attr id don't match GET mismatch", src_attr.id, dst_attr.id);
        }

        if (meta == NULL)
        {
            SWSS_LOG_THROW("unable to get metadata for object type %s, attribute %d",
                    lai_serialize_object_type(object_type).c_str(),
                    src_attr.id);
        }

        RETURN_ON_ERROR(transfer_attribute(meta->attrvaluetype, src_attr, dst_attr, countOnly));
    }

    return LAI_STATUS_SUCCESS;
}

lai_status_t transfer_stat(
        _In_ const lai_stat_metadata_t &meta,
        _In_ const lai_stat_value_t &src_stat,
        _In_ lai_stat_value_t &dst_stat)
{
    SWSS_LOG_ENTER();

    switch (meta.statvaluetype)
    {
        case LAI_STAT_VALUE_TYPE_UINT32:
            transfer_primitive(src_stat.u32, dst_stat.u32);
            break;

        case LAI_STAT_VALUE_TYPE_INT32:
            transfer_primitive(src_stat.s32, dst_stat.s32);
            break;

        case LAI_STAT_VALUE_TYPE_UINT64:
            transfer_primitive(src_stat.u64, dst_stat.u64);
            break;

        case LAI_STAT_VALUE_TYPE_INT64:
            transfer_primitive(src_stat.s64, dst_stat.s64);
            break;

        case LAI_STAT_VALUE_TYPE_DOUBLE:
            transfer_primitive(src_stat.d64, dst_stat.d64);
            break;

        default:
            return LAI_STATUS_NOT_IMPLEMENTED;
    }

    return LAI_STATUS_SUCCESS;
}

template<typename T>
static int compare_primitive(
        _In_ const T &item1,
        _In_ const T &item2)
{
    SWSS_LOG_ENTER();

    const unsigned char* ptr1 = reinterpret_cast<const unsigned char*>(&item1);
    const unsigned char* ptr2 = reinterpret_cast<const unsigned char*>(&item2);

    return memcmp(ptr1, ptr2, sizeof(T));
}

template<typename T>
static int compare_list(
        _In_ const T &list1,
        _In_ const T &list2)
{
    SWSS_LOG_ENTER();

    if (list1.count != list2.count)
    {
        return 1;
    }

    if (list1.count == 0)
    {
        return 0;
    }

    if (list1.list == NULL || list2.list == NULL)
    {
        return 1;
    }

    for (size_t i = 0; i < list1.count; i++)
    {
        if (compare_primitive(list1.list[i], list2.list[i]))
        {
            return 1;
        }
    }

    return 0;
}

int compare_attribute(
        _In_ lai_object_type_t object_type,
        _In_ const lai_attribute_t &attr1,
        _In_ const lai_attribute_t &attr2)
{
    SWSS_LOG_ENTER();

    int result = 1;

    if (attr1.id != attr2.id)
    {
        SWSS_LOG_THROW("attr1 (%d) vs attr2 (%d) attr-id don't match", attr1.id, attr2.id);
    }

    auto meta = lai_metadata_get_attr_metadata(object_type, attr1.id);

    if (meta == NULL)
    {
        SWSS_LOG_THROW("unable to get metadata for object type %s, attribute %d",
                lai_serialize_object_type(object_type).c_str(),
                attr1.id);
    }

    switch (meta->attrvaluetype)
    {
        case LAI_ATTR_VALUE_TYPE_BOOL:
            result = compare_primitive(attr1.value.booldata, attr2.value.booldata);
            break;

        case LAI_ATTR_VALUE_TYPE_CHARDATA:
            result = compare_primitive(attr1.value.chardata, attr2.value.chardata);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT8:
            result = compare_primitive(attr1.value.u8, attr2.value.u8);
            break;

        case LAI_ATTR_VALUE_TYPE_INT8:
            result = compare_primitive(attr1.value.s8, attr2.value.s8);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT16:
            result = compare_primitive(attr1.value.u16, attr2.value.u16);
            break;

        case LAI_ATTR_VALUE_TYPE_INT16:
            result = compare_primitive(attr1.value.s16, attr2.value.s16);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT32:
            result = compare_primitive(attr1.value.u32, attr2.value.u32);
            break;

        case LAI_ATTR_VALUE_TYPE_INT32:
            result = compare_primitive(attr1.value.s32, attr2.value.s32);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT64:
            result = compare_primitive(attr1.value.u64, attr2.value.u64);
            break;

        case LAI_ATTR_VALUE_TYPE_INT64:
            result = compare_primitive(attr1.value.s64, attr2.value.s64);
            break;

        case LAI_ATTR_VALUE_TYPE_DOUBLE:
            if (attr1.value.d64 - attr2.value.d64 > ACCURATE)
            {
                result = 1;
            }
            else if (attr2.value.d64 - attr1.value.d64 > ACCURATE)
            {
                result = -1;
            }
            else
            {
                result = 0;
            }
            break;

        case LAI_ATTR_VALUE_TYPE_POINTER:
            result = compare_primitive(attr1.value.ptr, attr2.value.ptr);
            break;

        case LAI_ATTR_VALUE_TYPE_OBJECT_ID:
            result = compare_primitive(attr1.value.oid, attr2.value.oid);
            break;

        case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:
            result = compare_list(attr1.value.objlist, attr2.value.objlist);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
            result = compare_list(attr1.value.u8list, attr2.value.u8list);
            break;

        case LAI_ATTR_VALUE_TYPE_INT8_LIST:
            result = compare_list(attr1.value.s8list, attr2.value.s8list);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
            result = compare_list(attr1.value.u16list, attr2.value.u16list);
            break;

        case LAI_ATTR_VALUE_TYPE_INT16_LIST:
            result = compare_list(attr1.value.s16list, attr2.value.s16list);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
            result = compare_list(attr1.value.u32list, attr2.value.u32list);
            break;

        case LAI_ATTR_VALUE_TYPE_INT32_LIST:
            result = compare_list(attr1.value.s32list, attr2.value.s32list);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT32_RANGE:
            result = compare_primitive(attr1.value.u32range, attr2.value.u32range);
            break;

        case LAI_ATTR_VALUE_TYPE_INT32_RANGE:
            result = compare_primitive(attr1.value.s32range, attr2.value.s32range);
            break;

        default:
            break;
    }

    return result;
}

int compare_stats(
        _In_ lai_object_type_t object_type,
        _In_ lai_stat_id_t stat_id,
        _In_ const lai_stat_value_t &stat1,
        _In_ const lai_stat_value_t &stat2)
{
    SWSS_LOG_ENTER();

    int result = 1;

    auto meta = lai_metadata_get_stat_metadata(object_type, stat_id);

    if (meta == NULL)
    {
        SWSS_LOG_THROW("unable to get metadata for object type %s, stat %d",
                lai_serialize_object_type(object_type).c_str(),
                stat_id);
    }

    switch (meta->statvaluetype)
    {
        case LAI_STAT_VALUE_TYPE_INT32:
            result = compare_primitive(stat1.s32, stat2.s32);
            break;

        case LAI_STAT_VALUE_TYPE_UINT32:
            result = compare_primitive(stat1.u32, stat2.u32);
            break;

        case LAI_STAT_VALUE_TYPE_INT64:
            result = compare_primitive(stat1.s64, stat2.s64);
            break;

        case LAI_STAT_VALUE_TYPE_UINT64:
            result = compare_primitive(stat1.u64, stat2.u64);
            break;

        case LAI_STAT_VALUE_TYPE_DOUBLE:
            if (stat1.d64 - stat2.d64 > ACCURATE)
            {
                result = 1;
            }
            else if (stat2.d64 - stat1.d64 > ACCURATE)
            {
                result = -1;
            }
            else
            {
                result = 0;
            }
            break;

        default:
            break;
    }

    return result;
}

lai_status_t inc_stat(
        _In_ const lai_stat_metadata_t &meta,
        _In_ lai_stat_value_t &stat,
        _In_ const lai_stat_value_t &increment)
{
    SWSS_LOG_ENTER();

    switch (meta.statvaluetype)
    {
        case LAI_STAT_VALUE_TYPE_UINT32:
            stat.u32 += increment.u32;
            break;

        case LAI_STAT_VALUE_TYPE_INT32:
            stat.s32 += increment.s32;
            break;

        case LAI_STAT_VALUE_TYPE_UINT64:
            stat.u64 += increment.u64;
            break;

        case LAI_STAT_VALUE_TYPE_INT64:
            stat.s64 += increment.s64;
            break;

        case LAI_STAT_VALUE_TYPE_DOUBLE:
            stat.d64 += increment.d64;
            break;

        default:
            return LAI_STATUS_NOT_IMPLEMENTED;
    }

    return LAI_STATUS_SUCCESS;
}

lai_status_t div_stat(
        _In_ const lai_stat_metadata_t &meta,
        _In_ lai_stat_value_t &stat,
        _In_ lai_stat_value_t &dividend,
        _In_ uint64_t divisor)
{
    SWSS_LOG_ENTER();

    switch (meta.statvaluetype)
    {
        case LAI_STAT_VALUE_TYPE_UINT32:
            stat.u32 = dividend.u32 / divisor;
            break;

        case LAI_STAT_VALUE_TYPE_INT32:
            stat.s32 = dividend.s32 / divisor;
            break;

        case LAI_STAT_VALUE_TYPE_UINT64:
            stat.u64 = dividend.u64 / divisor;
            break;

        case LAI_STAT_VALUE_TYPE_INT64:
            stat.s64 = dividend.s64 / divisor;
            break;

        case LAI_STAT_VALUE_TYPE_DOUBLE:
            stat.d64 = dividend.d64 / divisor;
            break;

        default:
            return LAI_STATUS_NOT_IMPLEMENTED;
    }

    return LAI_STATUS_SUCCESS;
}

// new methods

std::string lai_serialize_bool(
        _In_ bool b)
{
    SWSS_LOG_ENTER();

    return b ? "true" : "false";
}

#define CHAR_LEN 512

std::string lai_serialize_chardata(
        _In_ const char data[CHAR_LEN])
{
    SWSS_LOG_ENTER();

    std::string s;

    size_t len = strnlen(data, CHAR_LEN);

    for (size_t i = 0; i < len; ++i)
    {
        unsigned char c = (unsigned char)data[i];

        s += c;
    }

    return s;
}

template <typename T>
std::string lai_serialize_number(
        _In_ const T number,
        _In_ bool hex = false)
{
    SWSS_LOG_ENTER();

    if (hex)
    {
        char buf[32];

        snprintf(buf, sizeof(buf), "0x%" PRIx64, (uint64_t)number);

        return buf;
    }

    return std::to_string(number);
}

std::string lai_serialize_decimal(
        _In_ const double &value,
        _In_ int precision) 
{
    SWSS_LOG_ENTER();

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;

    std::string str = stream.str();
    if (str.find('.') != std::string::npos)
    {
        // remove trailing zeroes
        str = str.substr(0, str.find_last_not_of('0') + 1);
        // if the decimal point is now the last character, add a zero at the end
        if (str.find('.') == str.size() - 1)
        {
            str = str + "0";
        }
    }
    else
    {
        str = str + ".0";
    }

    return str;
}

std::string lai_serialize_string(
             _In_ lai_s8_list_t &value)
{
    std:: string result;
    if(value.list!=NULL)
	{
	 result = (char *)value.list;
	 delete[] value.list;
	}
    value.list = NULL;
	value.count = 0;
	return result;
}

std::string lai_serialize_enum(
        _In_ const int32_t value,
        _In_ const lai_enum_metadata_t* meta,
        _In_ const bool shortName)
{
    SWSS_LOG_ENTER();

    if (meta == NULL)
    {
        return lai_serialize_number(value);
    }

    for (size_t i = 0; i < meta->valuescount; ++i)
    {
        if (meta->values[i] == value)
        {
            if (shortName) {
                return meta->valuesshortnames[i];
            } else {
                return meta->valuesnames[i];
            }
        }
    }

    SWSS_LOG_WARN("enum value %d not found in enum %s", value, meta->name);

    return lai_serialize_number(value);
}

std::string lai_serialize_enum_v2(
        _In_ const int32_t value,
        _In_ const lai_enum_metadata_t* meta)
{
    SWSS_LOG_ENTER();

    if (meta == NULL)
    {
        return lai_serialize_number(value);
    }

    for (size_t i = 0; i < meta->valuescount; ++i)
    {
        if (meta->values[i] == value)
        {
            return meta->valuesshortnames[i];
        }
    }

    SWSS_LOG_WARN("enum value %d not found in enum %s", value, meta->name);

    return lai_serialize_number(value);
}

std::string lai_serialize_number(
        _In_ const uint32_t number,
        _In_ bool hex)
{
    SWSS_LOG_ENTER();

    return lai_serialize_number<uint32_t>(number, hex);
}

std::string lai_serialize_number(
        _In_ const uint64_t number,
        _In_ bool hex)
{
    SWSS_LOG_ENTER();

    return lai_serialize_number<uint64_t>(number, hex);
}

std::string lai_serialize_number(
        _In_ const uint16_t number,
        _In_ bool hex)
{
    SWSS_LOG_ENTER();

    return lai_serialize_number<uint16_t>(number, hex);
}

std::string lai_serialize_number(
        _In_ const uint8_t number,
        _In_ bool hex)
{
    SWSS_LOG_ENTER();

    return lai_serialize_number<uint8_t>(number, hex);
}

std::string lai_serialize_attr_id(
        _In_ const lai_attr_metadata_t& meta)
{
    SWSS_LOG_ENTER();

    return meta.attridname;
}

std::string lai_serialize_attr_id_kebab_case(
        _In_ const lai_attr_metadata_t& meta)
{
    SWSS_LOG_ENTER();

    return meta.attridkebabname;
}

std::string lai_serialize_stat_id(
        _In_ const lai_stat_metadata_t& meta)
{
    SWSS_LOG_ENTER();

    return meta.statidname;
}

std::string lai_serialize_stat_id_kebab_case(
        _In_ const lai_stat_metadata_t& meta)
{
    SWSS_LOG_ENTER();

    return meta.statidkebabname;
}

std::string lai_serialize_stat_id_camel_case(
        _In_ const lai_stat_metadata_t& meta)
{
    SWSS_LOG_ENTER();

    return meta.statidcamelname;
}

std::string lai_serialize_status(
        _In_ const lai_status_t status)
{
    SWSS_LOG_ENTER();

    return lai_serialize_enum(status, &lai_metadata_enum_lai_status_t);
}

std::string lai_serialize_common_api(
        _In_ const lai_common_api_t common_api)
{
    SWSS_LOG_ENTER();

    return lai_serialize_enum(common_api, &lai_metadata_enum_lai_common_api_t);
}

std::string lai_serialize_object_type(
        _In_ const lai_object_type_t object_type)
{
    SWSS_LOG_ENTER();

    return lai_serialize_enum(object_type, &lai_metadata_enum_lai_object_type_t);
}

std::string lai_serialize_log_level(
        _In_ lai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    return lai_serialize_enum(log_level, &lai_metadata_enum_lai_log_level_t);
}

std::string lai_serialize_api(
        _In_ lai_api_t api)
{
    SWSS_LOG_ENTER();

    return lai_serialize_enum(api, &lai_metadata_enum_lai_api_t);
}

std::string lai_serialize_attr_value_type(
        _In_ const lai_attr_value_type_t attr_value_type)
{
    SWSS_LOG_ENTER();

    return lai_serialize_enum(attr_value_type, &lai_metadata_enum_lai_attr_value_type_t);
}

#define EMIT(x)        buf += sprintf(buf, x)
#define EMIT_QUOTE     EMIT("\"")
#define EMIT_KEY(k)    EMIT("\"" k "\":")
#define EMIT_NEXT_KEY(k) { EMIT(","); EMIT_KEY(k); }
#define EMIT_CHECK(expr, suffix) {                              \
    ret = (expr);                                               \
    if (ret < 0) {                                              \
        SWSS_LOG_THROW("failed to serialize " #suffix ""); }    \
    buf += ret; }
#define EMIT_QUOTE_CHECK(expr, suffix) {\
    EMIT_QUOTE; EMIT_CHECK(expr, suffix); EMIT_QUOTE; }

std::string lai_serialize_linecard_alarm(
        _In_ lai_object_id_t &linecard_id,
        _In_ lai_alarm_type_t &alarm_type,
        _In_ lai_alarm_info_t &alarm_info)
{
    SWSS_LOG_ENTER();

    json j;
    std::string str_resource,str_alarm_type,type_id;

    j["linecard_id"] = lai_serialize_object_id(linecard_id);
    j["time-created"] = lai_serialize_number(alarm_info.time_created);
    j["resource_oid"] = lai_serialize_object_id(alarm_info.resource_oid);
    j["text"] = lai_serialize_string(alarm_info.text);
    j["severity"] = lai_serialize_enum(alarm_info.severity,&lai_metadata_enum_lai_alarm_severity_t);	
    j["type-id"] = lai_serialize_enum(alarm_type,&lai_metadata_enum_lai_alarm_type_t);
    j["status"] = lai_serialize_enum(alarm_info.status,&lai_metadata_enum_lai_alarm_status_t);
    j["id"] = str_resource + "#" + type_id;
	    
    return j.dump();
}

std::string lai_serialize_linecard_oper_status(
        _In_ lai_object_id_t linecard_id,
        _In_ lai_oper_status_t status)
{
    SWSS_LOG_ENTER();

    json j;

    j["linecard_id"] = lai_serialize_object_id(linecard_id);
    j["status"] = lai_serialize_enum(status, &lai_metadata_enum_lai_oper_status_t);

    return j.dump();
}

std::string lai_serialize_linecard_shutdown_request(
        _In_ lai_object_id_t linecard_id)
{
    SWSS_LOG_ENTER();

    json j;

    j["linecard_id"] = lai_serialize_object_id(linecard_id);

    return j.dump();
}

std::string lai_serialize_pointer(
        _In_ lai_pointer_t ptr)
{
    SWSS_LOG_ENTER();

    return lai_serialize_number((uint64_t)ptr, true);
}

std::string lai_serialize_object_id(
        _In_ lai_object_id_t oid)
{
    SWSS_LOG_ENTER();

    char buf[32];

    snprintf(buf, sizeof(buf), "oid:0x%" PRIx64, oid);

    return buf;
}

template<typename T, typename F>
std::string lai_serialize_list(
        _In_ const T& list,
        _In_ bool countOnly,
        F serialize_item)
{
    SWSS_LOG_ENTER();

    std::string s = lai_serialize_number(list.count);

    if (countOnly)
    {
        return s;
    }

    if (list.list == NULL || list.count == 0)
    {
        return s + ":null";
    }

    std::string l;

    for (uint32_t i = 0; i < list.count; ++i)
    {
        l += serialize_item(list.list[i]);

        if (i != list.count -1)
        {
            l += ",";
        }
    }

    return s + ":" + l;

}

std::string lai_serialize_enum_list(
        _In_ const lai_s32_list_t& list,
        _In_ const lai_enum_metadata_t* meta,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    return lai_serialize_list(list, countOnly, [&](int32_t item) { return lai_serialize_enum(item, meta);} );
}

std::string lai_serialize_oid_list(
        _In_ const lai_object_list_t &list,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    return lai_serialize_list(list, countOnly, [&](lai_object_id_t item) { return lai_serialize_object_id(item);} );
}

template <typename T>
std::string lai_serialize_number_list(
        _In_ const T& list,
        _In_ bool countOnly,
        _In_ bool hex = false)
{
    SWSS_LOG_ENTER();

    return lai_serialize_list(list, countOnly, [&](decltype(*list.list)& item) { return lai_serialize_number(item, hex);} );
}

template <typename T>
std::string lai_serialize_range(
        _In_ const T& range)
{
    SWSS_LOG_ENTER();

    return lai_serialize_number(range.min) + "," + lai_serialize_number(range.max);
}

std::string lai_serialize_hex_binary(
        _In_ const void *buffer,
        _In_ size_t length)
{
    SWSS_LOG_ENTER();

    std::string s;

    if (buffer == NULL || length == 0)
    {
        return s;
    }

    s.resize(2 * length, '0');

    const unsigned char *input = static_cast<const unsigned char *>(buffer);
    char *output = &s[0];

    for (size_t i = 0; i < length; i++)
    {
        snprintf(&output[i * 2], 3, "%02X", input[i]);
    }

    return s;
}

std::string lai_serialize_attr_value(
        _In_ const lai_attr_metadata_t& meta,
        _In_ const lai_attribute_t &attr,
        _In_ const bool countOnly,
        _In_ const bool shortName)
{
    SWSS_LOG_ENTER();

    switch (meta.attrvaluetype)
    {
        case LAI_ATTR_VALUE_TYPE_BOOL:
            return lai_serialize_bool(attr.value.booldata);

        case LAI_ATTR_VALUE_TYPE_CHARDATA:
            return lai_serialize_chardata(attr.value.chardata);

        case LAI_ATTR_VALUE_TYPE_UINT8:
            return lai_serialize_number(attr.value.u8);

        case LAI_ATTR_VALUE_TYPE_INT8:
            return lai_serialize_number(attr.value.s8);

        case LAI_ATTR_VALUE_TYPE_UINT16:
            return lai_serialize_number(attr.value.u16);

        case LAI_ATTR_VALUE_TYPE_INT16:
            return lai_serialize_number(attr.value.s16);

        case LAI_ATTR_VALUE_TYPE_UINT32:
            return lai_serialize_number(attr.value.u32);

        case LAI_ATTR_VALUE_TYPE_INT32:
            return lai_serialize_enum(attr.value.s32, meta.enummetadata, shortName);

        case LAI_ATTR_VALUE_TYPE_UINT64:
            return lai_serialize_number(attr.value.u64);

        case LAI_ATTR_VALUE_TYPE_INT64:
            return lai_serialize_number(attr.value.s64);

        case LAI_ATTR_VALUE_TYPE_DOUBLE:
            return lai_serialize_decimal(attr.value.d64);

        case LAI_ATTR_VALUE_TYPE_POINTER:
            return lai_serialize_pointer(attr.value.ptr);

        case LAI_ATTR_VALUE_TYPE_OBJECT_ID:
            return lai_serialize_object_id(attr.value.oid);

        case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:
            return lai_serialize_oid_list(attr.value.objlist, countOnly);

        case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
            return lai_serialize_number_list(attr.value.u8list, countOnly);

        case LAI_ATTR_VALUE_TYPE_INT8_LIST:
            return lai_serialize_number_list(attr.value.s8list, countOnly);

        case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
            return lai_serialize_number_list(attr.value.u16list, countOnly);

        case LAI_ATTR_VALUE_TYPE_INT16_LIST:
            return lai_serialize_number_list(attr.value.s16list, countOnly);

        case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
            return lai_serialize_number_list(attr.value.u32list, countOnly);

        case LAI_ATTR_VALUE_TYPE_INT32_LIST:
            return lai_serialize_enum_list(attr.value.s32list, meta.enummetadata, countOnly);

        case LAI_ATTR_VALUE_TYPE_UINT32_RANGE:
            return lai_serialize_range(attr.value.u32range);

        case LAI_ATTR_VALUE_TYPE_INT32_RANGE:
            return lai_serialize_range(attr.value.s32range);

        default:
            SWSS_LOG_THROW("FATAL: invalid serialization type %d", meta.attrvaluetype);
    }
}

std::string lai_serialize_stat_value(
        _In_ const lai_stat_metadata_t &meta,
        _In_ const lai_stat_value_t &stat)
{
    SWSS_LOG_ENTER();

    switch (meta.statvaluetype)
    {
        case LAI_STAT_VALUE_TYPE_UINT32:
            return lai_serialize_number(stat.u32);

        case LAI_STAT_VALUE_TYPE_INT32:
            return lai_serialize_number(stat.s32);

        case LAI_STAT_VALUE_TYPE_UINT64:
            return lai_serialize_number(stat.u64);

        case LAI_STAT_VALUE_TYPE_INT64:
            return lai_serialize_number(stat.s64);

        case LAI_STAT_VALUE_TYPE_DOUBLE:
        {
            int precision = 2;
            if (meta.statvalueprecision == LAI_STAT_VALUE_PRECISION_1)
            {
                precision = 1;
            }
            else if (meta.statvalueprecision == LAI_STAT_VALUE_PRECISION_18)
            {
                precision = 18;
            }
            return lai_serialize_decimal(stat.d64, precision);
        }

        default:
            SWSS_LOG_THROW("FATAL: invalid serialization type %d", meta.statvaluetype);
    }
}

std::string lai_serialize_object_meta_key(
        _In_ const lai_object_meta_key_t& meta_key)
{
    SWSS_LOG_ENTER();

    std::string key;

    if (meta_key.objecttype == LAI_OBJECT_TYPE_NULL || meta_key.objecttype >= LAI_OBJECT_TYPE_EXTENSIONS_MAX)
    {
        SWSS_LOG_THROW("invalid object type value %s", lai_serialize_object_type(meta_key.objecttype).c_str());
    }

    const auto& meta = lai_metadata_get_object_type_info(meta_key.objecttype);

    if (meta->isnonobjectid)
    {
        SWSS_LOG_THROW("object %s is non object id, not supported yet, FIXME",
                lai_serialize_object_type(meta->objecttype).c_str());
    }

    key = lai_serialize_object_id(meta_key.objectkey.key.object_id);

    key = lai_serialize_object_type(meta_key.objecttype) + ":" + key;

    SWSS_LOG_DEBUG("%s", key.c_str());

    return key;
}

#define SYNCD_INIT_VIEW     "INIT_VIEW"
#define SYNCD_APPLY_VIEW    "APPLY_VIEW"
#define SYNCD_INSPECT_ASIC  "SYNCD_INSPECT_ASIC"

std::string lai_serialize(
        _In_ const lai_redis_notify_syncd_t& value)
{
    SWSS_LOG_ENTER();

    switch (value)
    {
        case LAI_REDIS_NOTIFY_SYNCD_INIT_VIEW:
            return SYNCD_INIT_VIEW;

        case LAI_REDIS_NOTIFY_SYNCD_APPLY_VIEW:
            return SYNCD_APPLY_VIEW;

        case LAI_REDIS_NOTIFY_SYNCD_INSPECT_ASIC:
            return SYNCD_INSPECT_ASIC;

        default:

            SWSS_LOG_WARN("unknown value on lai_redis_notify_syncd_t: %d", value);

            return std::to_string(value);
    }
}

#define REDIS_COMMUNICATION_MODE_REDIS_ASYNC_STRING "redis_async"
#define REDIS_COMMUNICATION_MODE_REDIS_SYNC_STRING  "redis_sync"

std::string lai_serialize_redis_communication_mode(
        _In_ lai_redis_communication_mode_t value)
{
    SWSS_LOG_ENTER();

    switch (value)
    {
        case LAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC:
            return REDIS_COMMUNICATION_MODE_REDIS_ASYNC_STRING;

        case LAI_REDIS_COMMUNICATION_MODE_REDIS_SYNC:
            return REDIS_COMMUNICATION_MODE_REDIS_SYNC_STRING;

        default:

            SWSS_LOG_WARN("unknown value on lai_redis_communication_mode_t: %d", value);

            return std::to_string(value);
    }
}

std::string lai_serialize_ocm_spectrum_power(
        _In_ lai_spectrum_power_t ocm_result)
{
    SWSS_LOG_ENTER();

    return lai_serialize_number(ocm_result.lower_frequency) + "#" +
           lai_serialize_number(ocm_result.upper_frequency) + "#" +
           lai_serialize_decimal(ocm_result.power);
}

std::string lai_serialize_ocm_spectrum_power_list(
        _In_ lai_spectrum_power_list_t& list)
{
    SWSS_LOG_ENTER();

    return lai_serialize_list(list, false, [&](lai_spectrum_power_t item) { return lai_serialize_ocm_spectrum_power(item); });
}

std::string lai_serialize_otdr_event(
        _In_ lai_otdr_event_t event)
{
    SWSS_LOG_ENTER();

    return lai_serialize_enum(event.type, &lai_metadata_enum_lai_otdr_event_type_t) + "#" +
           lai_serialize_decimal(event.length) + "#" +
           lai_serialize_decimal(event.loss) + "#" +
           lai_serialize_decimal(event.reflection) + "#" +
           lai_serialize_decimal(event.accumulate_loss);
}

std::string lai_serialize_otdr_event_list(
        _In_ lai_otdr_event_list_t &list)
{
    SWSS_LOG_ENTER();

    return lai_serialize_list(list, false, [&](lai_otdr_event_t item) { return lai_serialize_otdr_event(item); });
}

// deserialize

void lai_deserialize_bool(
        _In_ const std::string& s,
        _Out_ bool& b)
{
    SWSS_LOG_ENTER();

    if (s == "true")
    {
        b = true;
        return;
    }

    if (s == "false")
    {
        b = false;
        return;
    }

    SWSS_LOG_THROW("failed to deserialize '%s' as bool", s.c_str());
}

void lai_deserialize_chardata(
        _In_ const std::string& s,
        _Out_ char chardata[CHAR_LEN])
{
    SWSS_LOG_ENTER();

    memset(chardata, 0, CHAR_LEN);

    size_t len = s.length();

    if (len > CHAR_LEN)
    {
        SWSS_LOG_THROW("invalid chardata %s", s.c_str());
    }

    memcpy(chardata, s.data(), len);
}

template<typename T>
void lai_deserialize_number(
        _In_ const std::string& s,
        _Out_ T& number,
        _In_ bool hex = false)
{
    SWSS_LOG_ENTER();

    errno = 0;

    char *endptr = NULL;

    number = (T)strtoull(s.c_str(), &endptr, hex ? 16 : 10);

    if (errno != 0 || endptr != s.c_str() + s.length())
    {
        SWSS_LOG_THROW("invalid number %s", s.c_str());
    }
}

void lai_deserialize_number(
        _In_ const std::string& s,
        _Out_ uint32_t& number,
        _In_ bool hex)
{
    SWSS_LOG_ENTER();

    lai_deserialize_number<uint32_t>(s, number, hex);
}

void lai_deserialize_number(
        _In_ const std::string& s,
        _Out_ uint64_t& number,
        _In_ bool hex)
{
    SWSS_LOG_ENTER();

    lai_deserialize_number<uint64_t>(s, number, hex);
}

void lai_deserialize_decimal(
        _In_ const std::string& s,
        _Out_ double& value)
{
    SWSS_LOG_ENTER();

    char *endptr = NULL;
    value = strtod(s.c_str(), &endptr);
}

void lai_deserialize_string(
      _In_ const std::string& s,
      _Out_ lai_s8_list_t &value)
{
  value.count = s.length();
  value.list =new int8_t[value.count+1];
  strncpy((char *)value.list,s.c_str(),value.count+1);
}
void lai_deserialize_enum(
        _In_ const std::string& s,
        _In_ const lai_enum_metadata_t *meta,
        _Out_ int32_t& value)
{
    SWSS_LOG_ENTER();

    if (meta == NULL)
    {
        return lai_deserialize_number(s, value);
    }

    for (size_t i = 0; i < meta->valuescount; ++i)
    {
        if (strcmp(s.c_str(), meta->valuesnames[i]) == 0 ||
            strcmp(s.c_str(), meta->valuesshortnames[i]) == 0)
        {
            value = meta->values[i];
            return;
        }
    }

    // check depreacated values if present
    if (meta->ignorevaluesnames)
    {
        // this can happen when we deserialize older LAI values

        for (size_t i = 0; meta->ignorevaluesnames[i] != NULL; i++)
        {
            if (strcmp(s.c_str(), meta->ignorevaluesnames[i]) == 0)
            {
                SWSS_LOG_NOTICE("translating depreacated/ignored enum value: %s", s.c_str());

                value = meta->ignorevalues[i];
                return;
            }
        }
    }

    SWSS_LOG_WARN("enum %s not found in enum %s", s.c_str(), meta->name);

    lai_deserialize_number(s, value);
}

void lai_deserialize_object_id(
        _In_ const std::string& s,
        _Out_ lai_object_id_t& oid)
{
    SWSS_LOG_ENTER();

    if (s.find("oid:0x") != 0)
    {
        SWSS_LOG_THROW("invalid oid %s", s.c_str());
    }

    errno = 0;

    char *endptr = NULL;

    oid = (lai_object_id_t)strtoull(s.c_str()+4, &endptr, 16);

    if (errno != 0 || endptr != s.c_str() + s.length())
    {
        SWSS_LOG_THROW("invalid oid %s", s.c_str());
    }
}

template<typename T, typename F>
void lai_deserialize_list(
        _In_ const std::string& s,
        _Out_ T& list,
        _In_ bool countOnly,
        F deserialize_item)
{
    SWSS_LOG_ENTER();

    if (countOnly)
    {
        lai_deserialize_number(s, list.count);
        return;
    }

    auto pos = s.find(":");

    if (pos == std::string::npos)
    {
        SWSS_LOG_THROW("invalid list %s", s.c_str());
    }

    std::string scount = s.substr(0, pos);

    lai_deserialize_number(scount, list.count);

    std::string slist = s.substr(pos + 1);

    if (slist == "null")
    {
        list.list = NULL;
        return;
    }

    auto tokens = swss::tokenize(slist, ',');

    if (tokens.size() != list.count)
    {
        SWSS_LOG_THROW("invalid list count %zu != %u", tokens.size(), list.count);
    }

    // list.list = lai_alloc_list(list.count, list);
    list.list = lai_alloc_n_of_ptr_type(list.count, list.list);

    for (uint32_t i = 0; i < list.count; ++i)
    {
        deserialize_item(tokens[i], list.list[i]);
    }
}

void lai_deserialize_oid_list(
        _In_ const std::string& s,
        _Out_ lai_object_list_t& objlist,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    lai_deserialize_list(s, objlist, countOnly, [&](const std::string sitem, lai_object_id_t& item) { lai_deserialize_object_id(sitem, item);} );
}

void lai_deserialize_enum_list(
        _In_ const std::string& s,
        _In_ const lai_enum_metadata_t* meta,
        _Out_ lai_s32_list_t& list,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    lai_deserialize_list(s, list, countOnly, [&](const std::string sitem, int32_t& item) { lai_deserialize_enum(sitem, meta, item);} );
}

template <typename T>
void lai_deserialize_number_list(
        _In_ const std::string& s,
        _Out_ T& list,
        _In_ bool countOnly,
        _In_ bool hex = false)
{
    SWSS_LOG_ENTER();

    lai_deserialize_list(s, list, countOnly, [&](const std::string sitem, decltype(*list.list)& item) { lai_deserialize_number(sitem, item, hex);} );
}

void lai_deserialize_pointer(
        _In_ const std::string& s,
        _Out_ lai_pointer_t& ptr)
{
    SWSS_LOG_ENTER();

    lai_deserialize_number(s, (uintptr_t &)ptr, true);
}

template <typename T>
void lai_deserialize_range(
        _In_ const std::string& s,
        _Out_ T& range)
{
    SWSS_LOG_ENTER();

    auto tokens = swss::tokenize(s, ',');

    if (tokens.size() != 2)
    {
        SWSS_LOG_THROW("invalid range %s", s.c_str());
    }

    lai_deserialize_number(tokens[0], range.min);
    lai_deserialize_number(tokens[1], range.max);
}

void lai_deserialize_hex_binary(
        _In_ const std::string &s,
        _Out_ void *buffer,
        _In_ size_t length)
{
    SWSS_LOG_ENTER();

    if (s.length() % 2 != 0)
    {
        SWSS_LOG_THROW("Invalid hex string %s", s.c_str());
    }

    if (s.length() > (length * 2))
    {
        SWSS_LOG_THROW("Buffer length isn't sufficient");
    }

    size_t buffer_cur = 0;
    size_t hex_cur = 0;
    unsigned char *output = static_cast<unsigned char *>(buffer);

    while (hex_cur < s.length())
    {
        const char temp_buffer[] = { s[hex_cur], s[hex_cur + 1], 0 };
        unsigned int value = -1;

        if (sscanf(temp_buffer, "%X", &value) <= 0 || value > 0xff)
        {
            SWSS_LOG_THROW("Invalid hex string %s", temp_buffer);
        }

        output[buffer_cur] = static_cast<unsigned char>(value);
        hex_cur += 2;
        buffer_cur += 1;
    }
}

template<typename T>
void lai_deserialize_hex_binary(
        _In_ const std::string &s,
        _Out_ T &value)
{
    SWSS_LOG_ENTER();

    return lai_deserialize_hex_binary(s, &value, sizeof(T));
}

void lai_deserialize_attr_value(
        _In_ const std::string& s,
        _In_ const lai_attr_metadata_t& meta,
        _Out_ lai_attribute_t &attr,
        _In_ const bool countOnly)
{
    SWSS_LOG_ENTER();

    memset(&attr.value, 0, sizeof(attr.value));

    switch (meta.attrvaluetype)
    {
        case LAI_ATTR_VALUE_TYPE_BOOL:
            return lai_deserialize_bool(s, attr.value.booldata);

        case LAI_ATTR_VALUE_TYPE_CHARDATA:
            return lai_deserialize_chardata(s, attr.value.chardata);

        case LAI_ATTR_VALUE_TYPE_UINT8:
            return lai_deserialize_number(s, attr.value.u8);

        case LAI_ATTR_VALUE_TYPE_INT8:
            return lai_deserialize_number(s, attr.value.s8);

        case LAI_ATTR_VALUE_TYPE_UINT16:
            return lai_deserialize_number(s, attr.value.u16);

        case LAI_ATTR_VALUE_TYPE_INT16:
            return lai_deserialize_number(s, attr.value.s16);

        case LAI_ATTR_VALUE_TYPE_UINT32:
            return lai_deserialize_number(s, attr.value.u32);

        case LAI_ATTR_VALUE_TYPE_INT32:
            return lai_deserialize_enum(s, meta.enummetadata, attr.value.s32);

        case LAI_ATTR_VALUE_TYPE_UINT64:
            return lai_deserialize_number(s, attr.value.u64);

        case LAI_ATTR_VALUE_TYPE_INT64:
            return lai_deserialize_number(s, attr.value.s64);

        case LAI_ATTR_VALUE_TYPE_DOUBLE:
            return lai_deserialize_decimal(s, attr.value.d64);

        case LAI_ATTR_VALUE_TYPE_POINTER:
            return lai_deserialize_pointer(s, attr.value.ptr);

        case LAI_ATTR_VALUE_TYPE_OBJECT_ID:
            return lai_deserialize_object_id(s, attr.value.oid);

        case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:
            return lai_deserialize_oid_list(s, attr.value.objlist, countOnly);

        case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
            return lai_deserialize_number_list(s, attr.value.u8list, countOnly);

        case LAI_ATTR_VALUE_TYPE_INT8_LIST:
            return lai_deserialize_number_list(s, attr.value.s8list, countOnly);

        case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
            return lai_deserialize_number_list(s, attr.value.u16list, countOnly);

        case LAI_ATTR_VALUE_TYPE_INT16_LIST:
            return lai_deserialize_number_list(s, attr.value.s16list, countOnly);

        case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
            return lai_deserialize_number_list(s, attr.value.u32list, countOnly);

        case LAI_ATTR_VALUE_TYPE_INT32_LIST:
            return lai_deserialize_enum_list(s, meta.enummetadata, attr.value.s32list, countOnly);

        case LAI_ATTR_VALUE_TYPE_UINT32_RANGE:
            return lai_deserialize_range(s, attr.value.u32range);

        case LAI_ATTR_VALUE_TYPE_INT32_RANGE:
            return lai_deserialize_range(s, attr.value.s32range);

        default:
            SWSS_LOG_THROW("deserialize type %d is not supported yet FIXME", meta.attrvaluetype);
    }
}

void lai_deserialize_status(
        _In_ const std::string& s,
        _Out_ lai_status_t& status)
{
    SWSS_LOG_ENTER();

    lai_deserialize_enum(s, &lai_metadata_enum_lai_status_t, status);
}

void lai_deserialize_linecard_oper_status(
        _In_ const std::string& s,
        _Out_ lai_object_id_t &linecard_id,
        _Out_ lai_oper_status_t& status)
{
    SWSS_LOG_ENTER();

    json j = json::parse(s);

    lai_deserialize_object_id(j["linecard_id"], linecard_id);
    lai_deserialize_enum(j["status"], &lai_metadata_enum_lai_oper_status_t, (int32_t&)status);
}

void lai_deserialize_linecard_shutdown_request(
        _In_ const std::string& s,
        _Out_ lai_object_id_t &linecard_id)
{
    SWSS_LOG_ENTER();

    json j = json::parse(s);

    lai_deserialize_object_id(j["linecard_id"], linecard_id);
}

void lai_deserialize_linecard_alarm(
		_In_ const std::string& s,
		_Out_ lai_object_id_t &linecard_id,
		_Out_ lai_alarm_type_t &alarm_type,
		_Out_ lai_alarm_info_t &alarm_info)
{
    SWSS_LOG_ENTER();

    int32_t temp_value=0;
    json j = json::parse(s);

    lai_deserialize_object_id(j["linecard_id"], linecard_id);
    lai_deserialize_number(j["time-created"], alarm_info.time_created);
    lai_deserialize_object_id(j["resource_oid"], alarm_info.resource_oid);
    lai_deserialize_string(j["text"],alarm_info.text);
    lai_deserialize_enum(j["severity"],&lai_metadata_enum_lai_alarm_severity_t,(int32_t &)temp_value);
    alarm_info.severity = (lai_alarm_severity_t)temp_value;
    lai_deserialize_enum(j["type-id"],&lai_metadata_enum_lai_alarm_type_t,(int32_t &)alarm_type);
    lai_deserialize_enum(j["status"],&lai_metadata_enum_lai_alarm_status_t,temp_value);
    alarm_info.status = (lai_alarm_status_t)temp_value;    
}

void lai_deserialize_object_type(
        _In_ const std::string& s,
        _Out_ lai_object_type_t& object_type)
{
    SWSS_LOG_ENTER();

    lai_deserialize_enum(s, &lai_metadata_enum_lai_object_type_t, (int32_t&)object_type);
}

void lai_deserialize_log_level(
        _In_ const std::string& s,
        _Out_ lai_log_level_t& log_level)
{
    SWSS_LOG_ENTER();

    lai_deserialize_enum(s, &lai_metadata_enum_lai_log_level_t, (int32_t&)log_level);
}

void lai_deserialize_api(
        _In_ const std::string& s,
        _Out_ lai_api_t& api)
{
    SWSS_LOG_ENTER();

    lai_deserialize_enum(s, &lai_metadata_enum_lai_api_t, (int32_t&)api);
}

#define EXPECT(x) { \
    if (strncmp(buf, x, sizeof(x) - 1) == 0) { buf += sizeof(x) - 1; }  \
    else { \
        SWSS_LOG_THROW("failed to expect %s on %s", x, buf); } }
#define EXPECT_QUOTE     EXPECT("\"")
#define EXPECT_KEY(k)    EXPECT("\"" k "\":")
#define EXPECT_NEXT_KEY(k) { EXPECT(","); EXPECT_KEY(k); }
#define EXPECT_CHECK(expr, suffix) {                                    \
    ret = (expr);                                                       \
    if (ret < 0) {                                                      \
        SWSS_LOG_THROW("failed to deserialize " #suffix ""); }          \
    buf += ret; }
#define EXPECT_QUOTE_CHECK(expr, suffix) {\
    EXPECT_QUOTE; EXPECT_CHECK(expr, suffix); EXPECT_QUOTE; }

void lai_deserialize_attr_id(
        _In_ const std::string& s,
        _Out_ const lai_attr_metadata_t** meta)
{
    SWSS_LOG_ENTER();

    if (meta == NULL)
    {
        SWSS_LOG_THROW("meta pointer is null");
    }

    auto m = lai_metadata_get_attr_metadata_by_attr_id_name(s.c_str());

    if (m == NULL)
    {
        SWSS_LOG_THROW("invalid attr id: %s", s.c_str());
    }

    *meta = m;
}

void lai_deserialize_attr_id(
        _In_ const std::string& s,
        _Out_ lai_attr_id_t& attrid)
{
    SWSS_LOG_ENTER();

    const lai_attr_metadata_t *meta = NULL;

    lai_deserialize_attr_id(s, &meta);

    attrid = meta->attrid;
}

void lai_deserialize_stat_id(
        _In_ const std::string& s,
        _Out_ const lai_stat_metadata_t** meta)
{
    SWSS_LOG_ENTER();

    if (meta == NULL)
    {
        SWSS_LOG_THROW("meta pointer is null");
    }

    auto m = lai_metadata_get_stat_metadata_by_stat_id_name(s.c_str());

    if (m == NULL)
    {
        SWSS_LOG_THROW("invalid stat id: %s", s.c_str());
    }

    *meta = m;
}

void lai_deserialize_object_meta_key(
        _In_ const std::string &s,
        _Out_ lai_object_meta_key_t& meta_key)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_DEBUG("%s", s.c_str());

    const std::string &str_object_type = s.substr(0, s.find(":"));
    const std::string &str_object_id = s.substr(s.find(":") + 1);

    lai_deserialize_object_type(str_object_type, meta_key.objecttype);

    if (meta_key.objecttype == LAI_OBJECT_TYPE_NULL || meta_key.objecttype >= LAI_OBJECT_TYPE_EXTENSIONS_MAX)
    {
        SWSS_LOG_THROW("invalid object type value %s", lai_serialize_object_type(meta_key.objecttype).c_str());
    }

    const auto& meta = lai_metadata_get_object_type_info(meta_key.objecttype);

    if (meta->isnonobjectid)
    {
        SWSS_LOG_THROW("object %s is non object id, not supported yet, FIXME",
                lai_serialize_object_type(meta->objecttype).c_str());
    }

    lai_deserialize_object_id(str_object_id, meta_key.objectkey.key.object_id);
}

// deserialize free

void lai_deserialize_free_attribute_value(
        _In_ const lai_attr_value_type_t type,
        _In_ lai_attribute_t &attr)
{
    SWSS_LOG_ENTER();

    // if we allocated list, then we need to free it

    switch (type)
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
        case LAI_ATTR_VALUE_TYPE_OBJECT_ID:
            break;

        case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:
            lai_free_list(attr.value.objlist);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
            lai_free_list(attr.value.u8list);
            break;

        case LAI_ATTR_VALUE_TYPE_INT8_LIST:
            lai_free_list(attr.value.s8list);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
            lai_free_list(attr.value.u16list);
            break;

        case LAI_ATTR_VALUE_TYPE_INT16_LIST:
            lai_free_list(attr.value.s16list);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
            lai_free_list(attr.value.u32list);
            break;

        case LAI_ATTR_VALUE_TYPE_INT32_LIST:
            lai_free_list(attr.value.s32list);
            break;

        case LAI_ATTR_VALUE_TYPE_UINT32_RANGE:
        case LAI_ATTR_VALUE_TYPE_INT32_RANGE:
            break;

        default:
            SWSS_LOG_THROW("unsupported type %d on deserialize free, FIXME", type);
    }
}

// lairedis

void lai_deserialize(
        _In_ const std::string& s,
        _Out_ lai_redis_notify_syncd_t& value)
{
    SWSS_LOG_ENTER();

    if (s == SYNCD_INIT_VIEW)
    {
        value = LAI_REDIS_NOTIFY_SYNCD_INIT_VIEW;
    }
    else if (s == SYNCD_APPLY_VIEW)
    {
        value = LAI_REDIS_NOTIFY_SYNCD_APPLY_VIEW;
    }
    else if (s == SYNCD_INSPECT_ASIC)
    {
        value = LAI_REDIS_NOTIFY_SYNCD_INSPECT_ASIC;
    }
    else
    {
        SWSS_LOG_WARN("enum %s not found in lai_redis_notify_syncd_t", s.c_str());

        lai_deserialize_number(s, value);
    }
}

lai_redis_notify_syncd_t lai_deserialize_redis_notify_syncd(
        _In_ const std::string& s)
{
    SWSS_LOG_ENTER();

    lai_redis_notify_syncd_t value;

    lai_deserialize(s, value);

    return value;
}

void lai_deserialize_redis_communication_mode(
        _In_ const std::string& s,
        _Out_ lai_redis_communication_mode_t& value)
{
    SWSS_LOG_ENTER();

    if (s == REDIS_COMMUNICATION_MODE_REDIS_ASYNC_STRING)
    {
        value = LAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC;
    }
    else if (s == REDIS_COMMUNICATION_MODE_REDIS_SYNC_STRING)
    {
        value = LAI_REDIS_COMMUNICATION_MODE_REDIS_SYNC;
    }
    else
    {
        SWSS_LOG_THROW("enum '%s' not found in lai_redis_communication_mode_t", s.c_str());
    }
}

std::string lai_serialize_transceiver_stat(
        _In_ const lai_transceiver_stat_t stat)
{
    SWSS_LOG_ENTER();

    return lai_serialize_enum(stat, &lai_metadata_enum_lai_transceiver_stat_t);
}

int lai_deserialize_transceiver_stat(
    _In_ const char *buffer,
    _Out_ lai_transceiver_stat_t *transceiver_stat)
{
    SWSS_LOG_ENTER();

    return lai_deserialize_enum(buffer, &lai_metadata_enum_lai_transceiver_stat_t, (int*)transceiver_stat);
}

void lai_deserialize_ocm_spectrum_power(
    _In_ const std::string& s,
    _Out_ lai_spectrum_power_t& ocm_result)
{
    SWSS_LOG_ENTER();

    auto tokens = swss::tokenize(s, '#');

    if (tokens.size() != 3)
    {
        SWSS_LOG_THROW("invalid serialized spectrum power, %s", s.c_str());
    }

    lai_deserialize_number(tokens[0], ocm_result.lower_frequency);
    lai_deserialize_number(tokens[1], ocm_result.upper_frequency);
    lai_deserialize_decimal(tokens[2], ocm_result.power);
}

void lai_deserialize_ocm_spectrum_power_list(
    _In_ const std::string& s,
    _Out_ lai_spectrum_power_list_t& list)
{
    SWSS_LOG_ENTER();

    lai_deserialize_list(s, list, false, [&](const std::string sitem, lai_spectrum_power_t& item) { lai_deserialize_ocm_spectrum_power(sitem, item); });
}

void lai_deserialize_otdr_event(
    _In_ const std::string &s,
    _Out_ lai_otdr_event_t &event)
{
    SWSS_LOG_ENTER();

    auto tokens = swss::tokenize(s, '#');

    if (tokens.size() != 5)
    {
        SWSS_LOG_THROW("invalid serialized otdr event, %s", s.c_str());
    }

    lai_otdr_event_type_t &event_type = event.type;

    lai_deserialize_enum(tokens[0], &lai_metadata_enum_lai_otdr_event_type_t, (int32_t&)event_type);

    lai_deserialize_decimal(tokens[1], event.length);
    lai_deserialize_decimal(tokens[2], event.loss);
    lai_deserialize_decimal(tokens[3], event.reflection);
    lai_deserialize_decimal(tokens[4], event.accumulate_loss);
}

void lai_deserialize_otdr_event_list(
    _In_ const std::string& s,
    _Out_ lai_otdr_event_list_t& list)
{
    SWSS_LOG_ENTER();

    lai_deserialize_list(s, list, false, [&](const std::string sitem, lai_otdr_event_t &item) { lai_deserialize_otdr_event(sitem, item); });
}

