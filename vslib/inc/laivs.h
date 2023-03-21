#pragma once

extern "C" {
#include "lai.h"
}

#define LAI_KEY_VS_LINECARD_TYPE              "LAI_VS_LINECARD_TYPE"
#define LAI_KEY_VS_LINECARD_LOCATION          "LAI_LINECARD_LOCATION"
#define LAI_KEY_VS_LAI_LINECARD_TYPE          "LAI_VS_LAI_LINECARD_TYPE"

#define LAI_VALUE_LAI_LINECARD_TYPE_NPU       "LAI_LINECARD_TYPE_NPU"
#define LAI_VALUE_LAI_LINECARD_TYPE_PHY       "LAI_LINECARD_TYPE_PHY"

#define LAI_VALUE_LAI_LINECARD_TYPE_P230C     "LAI_LINECARD_TYPE_P230C"

/**
 * @def LAI_KEY_VS_INTERFACE_LANE_MAP_FILE
 *
 * If specified in profile.ini it should point to eth interface to lane map.
 *
 * Example:
 * eth0:1,2,3,4
 * eth1:5,6,7,8
 *
 * TODO must support hardware info for multiple linecards
 */
#define LAI_KEY_VS_INTERFACE_LANE_MAP_FILE  "LAI_VS_INTERFACE_LANE_MAP_FILE"

/**
 * @def LAI_KEY_VS_RESOURCE_LIMITER_FILE
 *
 * File with resource limitations for object type create.
 *
 * Example:
 * LAI_OBJECT_TYPE_ACL_TABLE=3
 */
#define LAI_KEY_VS_RESOURCE_LIMITER_FILE    "LAI_VS_RESOURCE_LIMITER_FILE"

/**
 * @def LAI_KEY_VS_INTERFACE_FABRIC_LANE_MAP_FILE
 *
 * If specified in profile.ini it should point to fabric port to lane map.
 *
 * Example:
 * fabric0:1
 * fabric1:2
 *
 */
#define LAI_KEY_VS_INTERFACE_FABRIC_LANE_MAP_FILE  "LAI_VS_INTERFACE_FABRIC_LANE_MAP_FILE"

/**
 * @def LAI_KEY_VS_HOSTIF_USE_TAP_DEVICE
 *
 * Bool flag, (true/false). If set to true, then during create host interface
 * lai object also tap device will be created and mac address will be assigned.
 * For this operation root privileges will be required.
 *
 * By default this flag is set to false.
 */
#define LAI_KEY_VS_HOSTIF_USE_TAP_DEVICE      "LAI_VS_HOSTIF_USE_TAP_DEVICE"

/**
 * @def LAI_KEY_VS_CORE_PORT_INDEX_MAP_FILE
 *
 * For VOQ systems if specified in profile.ini it should point to eth interface to
 * core and core port index map as port name:core_index,core_port_index
 *
 * Example:
 * eth1:0,1
 * eth17:1,1
 *
 */
#define LAI_KEY_VS_CORE_PORT_INDEX_MAP_FILE  "LAI_VS_CORE_PORT_INDEX_MAP_FILE"

#define LAI_VALUE_VS_LINECARD_TYPE_P230C        "LAI_VS_LINECARD_TYPE_P230C"

/*
 * Values for LAI_KEY_BOOT_TYPE (defined in lailinecard.h)
 */

#define LAI_VALUE_VS_BOOT_TYPE_COLD "0"
#define LAI_VALUE_VS_BOOT_TYPE_WARM "1"
#define LAI_VALUE_VS_BOOT_TYPE_FAST "2"

/**
 * @def LAI_VS_UNITTEST_CHANNEL
 *
 * Notification channel for redis database.
 */
#define LAI_VS_UNITTEST_CHANNEL     "LAI_VS_UNITTEST_CHANNEL"

/**
 * @def LAI_VS_UNITTEST_SET_RO_OP
 *
 * Notification operation for "SET" READ_ONLY attribute.
 */
#define LAI_VS_UNITTEST_SET_RO_OP   "set_ro"

/**
 * @def LAI_VS_UNITTEST_SET_STATS
 *
 * Notification operation for "SET" stats on specific object.
 */
#define LAI_VS_UNITTEST_SET_STATS_OP      "set_stats"

/**
 * @def LAI_VS_UNITTEST_ENABLE
 *
 * Notification operation for enabling unittests.
 */
#define LAI_VS_UNITTEST_ENABLE_UNITTESTS  "enable_unittests"

typedef enum _lai_vs_linecard_attr_t
{
    /**
     * @brief Will enable metadata unittests.
     *
     * @type bool
     * @flags CREATE_AND_SET
     * @default false
     */
    LAI_VS_LINECARD_ATTR_META_ENABLE_UNITTESTS = LAI_LINECARD_ATTR_CUSTOM_RANGE_START,

    /**
     * @brief Will allow to set value that is read only.
     *
     * Unittests must be enabled.
     *
     * Value is the attribute to be allowed.
     *
     * @type lai_int32_t
     * @flags CREATE_AND_SET
     */
    LAI_VS_LINECARD_ATTR_META_ALLOW_READ_ONLY_ONCE,

} sau_vs_linecard_attr_t;
