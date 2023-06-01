#ifndef __LAI_SERIALIZE__
#define __LAI_SERIALIZE__

extern "C" {
#include "lai.h"
#include "laimetadata.h"
}

#include <iostream>
#include <fstream>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <iomanip>
#include <map>
#include <tuple>
#include <cstring>

#include "swss/logger.h"

#include "lairedis.h"

// util

lai_status_t transfer_attributes(
        _In_ lai_object_type_t object_type,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *src_attr_list,
        _In_ lai_attribute_t *dst_attr_list,
        _In_ bool countOnly = false);

lai_status_t transfer_stat(
        _In_ const lai_stat_metadata_t &meta,
        _In_ const lai_stat_value_t &src_stat,
        _In_ lai_stat_value_t &dst_stat);

int compare_attribute(
        _In_ lai_object_type_t object_type,
        _In_ const lai_attribute_t &attr1,
        _In_ const lai_attribute_t &attr2);

int compare_stats(
        _In_ lai_object_type_t object_type,
        _In_ lai_stat_id_t stat_id,
        _In_ const lai_stat_value_t &stat1,
        _In_ const lai_stat_value_t &stat2);

lai_status_t inc_stat(
        _In_ const lai_stat_metadata_t &meta,
        _In_ lai_stat_value_t &stat,
        _In_ const lai_stat_value_t &increment);

lai_status_t div_stat(
        _In_ const lai_stat_metadata_t &meta,
        _In_ lai_stat_value_t &stat,
        _In_ lai_stat_value_t &dividend,
        _In_ uint64_t divisor);

// serialize

std::string lai_serialize_object_type(
        _In_ const lai_object_type_t object_type);

std::string lai_serialize_object_id(
        _In_ const lai_object_id_t object_id);

std::string lai_serialize_log_level(
        _In_ const lai_log_level_t log_level);

std::string lai_serialize_api(
        _In_ const lai_api_t api);

std::string lai_serialize_attr_value_type(
        _In_ const lai_attr_value_type_t attr_value_type);

std::string lai_serialize_attr_value(
        _In_ const lai_attr_metadata_t& meta,
        _In_ const lai_attribute_t &attr,
        _In_ const bool countOnly = false,
        _In_ const bool shortName = false);

std::string lai_serialize_stat_value(
        _In_ const lai_stat_metadata_t& meta,
        _In_ const lai_stat_value_t &stat);

std::string lai_serialize_status(
        _In_ const lai_status_t status);

std::string lai_serialize_common_api(
        _In_ const lai_common_api_t common_api);

std::string lai_serialize_hex_binary(
        _In_ const void *buffer,
        _In_ size_t length);

template <typename T>
std::string lai_serialize_hex_binary(
        _In_ const T &value)
{
    SWSS_LOG_ENTER();

    return lai_serialize_hex_binary(&value, sizeof(T));
}

std::string lai_serialize_linecard_alarm(
        _In_ lai_object_id_t &linecard_id,
        _In_ lai_alarm_type_t &alarm_type,
        _In_ lai_alarm_info_t &alarm_info);

std::string lai_serialize_linecard_oper_status(
        _In_ lai_object_id_t linecard_id,
        _In_ lai_oper_status_t status);

std::string lai_serialize_linecard_shutdown_request(
        _In_ lai_object_id_t linecard_id);

std::string lai_serialize_enum(
        _In_ const int32_t value,
        _In_ const lai_enum_metadata_t* meta,
        _In_ const bool shortName = false);

std::string lai_serialize_enum_v2(
        _In_ const int32_t value,
        _In_ const lai_enum_metadata_t* meta);

std::string lai_serialize_number(
        _In_ uint32_t number,
        _In_ bool hex = false);

std::string lai_serialize_number(
        _In_ uint64_t number,
        _In_ bool hex = false);

std::string lai_serialize_number(
        _In_ uint16_t number,
        _In_ bool hex = false);

std::string lai_serialize_number(
        _In_ uint8_t number,
        _In_ bool hex = false);

std::string lai_serialize_decimal(
        _In_ const double &value,
        _In_ int precision = 2);

std::string lai_serialize_attr_id(
        _In_ const lai_attr_metadata_t& meta);

std::string lai_serialize_attr_id_kebab_case(
        _In_ const lai_attr_metadata_t& meta);

std::string lai_serialize_stat_id(
        _In_ const lai_stat_metadata_t& meta);

std::string lai_serialize_stat_id_kebab_case(
        _In_ const lai_stat_metadata_t& meta);

std::string lai_serialize_stat_id_camel_case(
        _In_ const lai_stat_metadata_t& meta);

std::string lai_serialize_object_meta_key(
        _In_ const lai_object_meta_key_t& meta_key);

// lairedis

std::string lai_serialize(
        _In_ const lai_redis_notify_syncd_t& value);

std::string lai_serialize_redis_communication_mode(
        _In_ lai_redis_communication_mode_t value);

// deserialize

void lai_deserialize_enum(
        _In_ const std::string& s,
        _In_ const lai_enum_metadata_t * meta,
        _Out_ int32_t& value);

void lai_deserialize_number(
        _In_ const std::string& s,
        _Out_ uint32_t& number,
        _In_ bool hex = false);

void lai_deserialize_number(
        _In_ const std::string& s,
        _Out_ uint64_t& number,
        _In_ bool hex = false);

void lai_deserialize_status(
        _In_ const std::string& s,
        _Out_ lai_status_t& status);

void lai_deserialize_linecard_oper_status(
        _In_ const std::string& s,
        _Out_ lai_object_id_t &linecard_id,
        _Out_ lai_oper_status_t& status);

void lai_deserialize_linecard_shutdown_request(
        _In_ const std::string& s,
        _Out_ lai_object_id_t &linecard_id);

void lai_deserialize_linecard_alarm(
        _In_ const std::string& s,
        _Out_ lai_object_id_t &linecard_id,
        _Out_ lai_alarm_type_t &alarm_type,
        _Out_ lai_alarm_info_t &alarm_info);

void lai_deserialize_object_type(
        _In_ const std::string& s,
        _Out_ lai_object_type_t& object_type);

void lai_deserialize_object_id(
        _In_ const std::string& s,
        _Out_ lai_object_id_t& oid);

void lai_deserialize_log_level(
        _In_ const std::string& s,
        _Out_ lai_log_level_t& log_level);

void lai_deserialize_api(
        _In_ const std::string& s,
        _Out_ lai_api_t& api);

void lai_deserialize_attr_value(
        _In_ const std::string& s,
        _In_ const lai_attr_metadata_t& meta,
        _Out_ lai_attribute_t &attr,
        _In_ const bool countOnly = false);

void lai_deserialize_attr_id(
        _In_ const std::string& s,
        _Out_ lai_attr_id_t &attrid);

void lai_deserialize_attr_id(
        _In_ const std::string& s,
        _In_ const lai_attr_metadata_t** meta);

void lai_deserialize_stat_id(
        _In_ const std::string& s,
        _In_ const lai_stat_metadata_t** meta);

void lai_deserialize_object_meta_key(
        _In_ const std::string &s,
        _Out_ lai_object_meta_key_t& meta_key);

// free methods

void lai_deserialize_free_attribute_value(
        _In_ const lai_attr_value_type_t type,
        _In_ lai_attribute_t &attr);

// lairedis

void lai_deserialize(
        _In_ const std::string& s,
        _Out_ lai_redis_notify_syncd_t& value);

lai_redis_notify_syncd_t lai_deserialize_redis_notify_syncd(
        _In_ const std::string& s);

void lai_deserialize_redis_communication_mode(
        _In_ const std::string& s,
        _Out_ lai_redis_communication_mode_t& value);

std::string lai_serialize_transceiver_stat(
        _In_ const lai_transceiver_stat_t stat);

int lai_deserialize_transceiver_stat(
    _In_ const char *buffer,
    _Out_ lai_transceiver_stat_t *transceiver_stat);

std::string lai_serialize_ocm_spectrum_power(
    _In_ lai_spectrum_power_t ocm_result);

std::string lai_serialize_ocm_spectrum_power_list(
    _In_ lai_spectrum_power_list_t& list);

std::string lai_serialize_otdr_event(
    _In_ lai_otdr_event_t event);

std::string lai_serialize_otdr_event_list(
    _In_ lai_otdr_event_list_t &list);

void lai_deserialize_ocm_spectrum_power(
    _In_ const std::string& s,
    _Out_ lai_spectrum_power_t& ocm_result);

void lai_deserialize_ocm_spectrum_power_list(
    _In_ const std::string& s,
    _Out_ lai_spectrum_power_list_t& list);

void lai_deserialize_otdr_event(
    _In_ const std::string &s,
    _Out_ lai_otdr_event_t &event);

void lai_deserialize_otdr_event_list(
    _In_ const std::string& s,
    _Out_ lai_otdr_event_list_t &list);

#endif // __LAI_SERIALIZE__
