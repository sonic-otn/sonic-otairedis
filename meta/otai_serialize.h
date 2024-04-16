#ifndef __OTAI_SERIALIZE__
#define __OTAI_SERIALIZE__

extern "C" {
#include "otai.h"
#include "otaimetadata.h"
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

#include "otairedis.h"

// util

otai_status_t transfer_attributes(
        _In_ otai_object_type_t object_type,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *src_attr_list,
        _In_ otai_attribute_t *dst_attr_list,
        _In_ bool countOnly = false);

otai_status_t transfer_stat(
        _In_ const otai_stat_metadata_t &meta,
        _In_ const otai_stat_value_t &src_stat,
        _In_ otai_stat_value_t &dst_stat);

int compare_attribute(
        _In_ otai_object_type_t object_type,
        _In_ const otai_attribute_t &attr1,
        _In_ const otai_attribute_t &attr2);

int compare_stats(
        _In_ otai_object_type_t object_type,
        _In_ otai_stat_id_t stat_id,
        _In_ const otai_stat_value_t &stat1,
        _In_ const otai_stat_value_t &stat2);

otai_status_t inc_stat(
        _In_ const otai_stat_metadata_t &meta,
        _In_ otai_stat_value_t &stat,
        _In_ const otai_stat_value_t &increment);

otai_status_t div_stat(
        _In_ const otai_stat_metadata_t &meta,
        _In_ otai_stat_value_t &stat,
        _In_ otai_stat_value_t &dividend,
        _In_ uint64_t divisor);

// serialize

std::string otai_serialize_object_type(
        _In_ const otai_object_type_t object_type);

std::string otai_serialize_object_id(
        _In_ const otai_object_id_t object_id);

std::string otai_serialize_log_level(
        _In_ const otai_log_level_t log_level);

std::string otai_serialize_api(
        _In_ const otai_api_t api);

std::string otai_serialize_attr_value_type(
        _In_ const otai_attr_value_type_t attr_value_type);

std::string otai_serialize_attr_value(
        _In_ const otai_attr_metadata_t& meta,
        _In_ const otai_attribute_t &attr,
        _In_ const bool countOnly = false,
        _In_ const bool shortName = false);

std::string otai_serialize_stat_value(
        _In_ const otai_stat_metadata_t& meta,
        _In_ const otai_stat_value_t &stat);

std::string otai_serialize_status(
        _In_ const otai_status_t status);

std::string otai_serialize_common_api(
        _In_ const otai_common_api_t common_api);

std::string otai_serialize_hex_binary(
        _In_ const void *buffer,
        _In_ size_t length);

template <typename T>
std::string otai_serialize_hex_binary(
        _In_ const T &value)
{
    SWSS_LOG_ENTER();

    return otai_serialize_hex_binary(&value, sizeof(T));
}

std::string otai_serialize_linecard_alarm(
        _In_ otai_object_id_t &linecard_id,
        _In_ otai_alarm_type_t &alarm_type,
        _In_ otai_alarm_info_t &alarm_info);

std::string otai_serialize_linecard_oper_status(
        _In_ otai_object_id_t linecard_id,
        _In_ otai_oper_status_t status);

std::string otai_serialize_linecard_shutdown_request(
        _In_ otai_object_id_t linecard_id);

std::string otai_serialize_enum(
        _In_ const int32_t value,
        _In_ const otai_enum_metadata_t* meta,
        _In_ const bool shortName = false);

std::string otai_serialize_enum_v2(
        _In_ const int32_t value,
        _In_ const otai_enum_metadata_t* meta);

std::string otai_serialize_number(
        _In_ uint32_t number,
        _In_ bool hex = false);

std::string otai_serialize_number(
        _In_ uint64_t number,
        _In_ bool hex = false);

std::string otai_serialize_number(
        _In_ uint16_t number,
        _In_ bool hex = false);

std::string otai_serialize_number(
        _In_ uint8_t number,
        _In_ bool hex = false);

std::string otai_serialize_decimal(
        _In_ const double &value,
        _In_ int precision = 2);

std::string otai_serialize_attr_id(
        _In_ const otai_attr_metadata_t& meta);

std::string otai_serialize_attr_id_kebab_case(
        _In_ const otai_attr_metadata_t& meta);

std::string otai_serialize_stat_id(
        _In_ const otai_stat_metadata_t& meta);

std::string otai_serialize_stat_id_kebab_case(
        _In_ const otai_stat_metadata_t& meta);

std::string otai_serialize_stat_id_camel_case(
        _In_ const otai_stat_metadata_t& meta);

std::string otai_serialize_object_meta_key(
        _In_ const otai_object_meta_key_t& meta_key);

// otairedis

std::string otai_serialize(
        _In_ const otai_redis_notify_syncd_t& value);

std::string otai_serialize_redis_communication_mode(
        _In_ otai_redis_communication_mode_t value);

// deserialize

void otai_deserialize_enum(
        _In_ const std::string& s,
        _In_ const otai_enum_metadata_t * meta,
        _Out_ int32_t& value);

void otai_deserialize_number(
        _In_ const std::string& s,
        _Out_ uint32_t& number,
        _In_ bool hex = false);

void otai_deserialize_number(
        _In_ const std::string& s,
        _Out_ uint64_t& number,
        _In_ bool hex = false);

void otai_deserialize_status(
        _In_ const std::string& s,
        _Out_ otai_status_t& status);

void otai_deserialize_linecard_oper_status(
        _In_ const std::string& s,
        _Out_ otai_object_id_t &linecard_id,
        _Out_ otai_oper_status_t& status);

void otai_deserialize_linecard_shutdown_request(
        _In_ const std::string& s,
        _Out_ otai_object_id_t &linecard_id);

void otai_deserialize_linecard_alarm(
        _In_ const std::string& s,
        _Out_ otai_object_id_t &linecard_id,
        _Out_ otai_alarm_type_t &alarm_type,
        _Out_ otai_alarm_info_t &alarm_info);

void otai_deserialize_object_type(
        _In_ const std::string& s,
        _Out_ otai_object_type_t& object_type);

void otai_deserialize_object_id(
        _In_ const std::string& s,
        _Out_ otai_object_id_t& oid);

void otai_deserialize_log_level(
        _In_ const std::string& s,
        _Out_ otai_log_level_t& log_level);

void otai_deserialize_api(
        _In_ const std::string& s,
        _Out_ otai_api_t& api);

void otai_deserialize_attr_value(
        _In_ const std::string& s,
        _In_ const otai_attr_metadata_t& meta,
        _Out_ otai_attribute_t &attr,
        _In_ const bool countOnly = false);

void otai_deserialize_attr_id(
        _In_ const std::string& s,
        _Out_ otai_attr_id_t &attrid);

void otai_deserialize_attr_id(
        _In_ const std::string& s,
        _In_ const otai_attr_metadata_t** meta);

void otai_deserialize_stat_id(
        _In_ const std::string& s,
        _In_ const otai_stat_metadata_t** meta);

void otai_deserialize_object_meta_key(
        _In_ const std::string &s,
        _Out_ otai_object_meta_key_t& meta_key);

// free methods

void otai_deserialize_free_attribute_value(
        _In_ const otai_attr_value_type_t type,
        _In_ otai_attribute_t &attr);

// otairedis

void otai_deserialize(
        _In_ const std::string& s,
        _Out_ otai_redis_notify_syncd_t& value);

otai_redis_notify_syncd_t otai_deserialize_redis_notify_syncd(
        _In_ const std::string& s);

void otai_deserialize_redis_communication_mode(
        _In_ const std::string& s,
        _Out_ otai_redis_communication_mode_t& value);

std::string otai_serialize_transceiver_stat(
        _In_ const otai_transceiver_stat_t stat);

int otai_deserialize_transceiver_stat(
    _In_ const char *buffer,
    _Out_ otai_transceiver_stat_t *transceiver_stat);

std::string otai_serialize_ocm_spectrum_power(
    _In_ otai_spectrum_power_t ocm_result);

std::string otai_serialize_ocm_spectrum_power_list(
    _In_ otai_spectrum_power_list_t& list);

std::string otai_serialize_otdr_event(
    _In_ otai_otdr_event_t event);

std::string otai_serialize_otdr_event_list(
    _In_ otai_otdr_event_list_t &list);

void otai_deserialize_ocm_spectrum_power(
    _In_ const std::string& s,
    _Out_ otai_spectrum_power_t& ocm_result);

void otai_deserialize_ocm_spectrum_power_list(
    _In_ const std::string& s,
    _Out_ otai_spectrum_power_list_t& list);

void otai_deserialize_otdr_event(
    _In_ const std::string &s,
    _Out_ otai_otdr_event_t &event);

void otai_deserialize_otdr_event_list(
    _In_ const std::string& s,
    _Out_ otai_otdr_event_list_t &list);

#endif // __OTAI_SERIALIZE__
