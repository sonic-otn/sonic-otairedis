#pragma once

/*
 * This header will contain definitions used by libotairedis and syncd as well
 * as libswsscommon (for producer/consumer) and lua scripts.
 */

#define ASIC_STATE_TABLE    "ASIC_STATE"
#define FLEX_COUNTER_GROUP_TABLE    "FLEX_COUNTER_GROUP_TABLE"
#define FLEX_COUNTER_TABLE    "FLEX_COUNTER_TABLE"
#define TEMP_PREFIX         "TEMP_"

/*
 * Asic state table commands. Those names are special and they will be used
 * inside swsscommon library LUA scripts to perform operations on redis
 * database.
 */

#define REDIS_ASIC_STATE_COMMAND_CREATE "create"
#define REDIS_ASIC_STATE_COMMAND_REMOVE "remove"
#define REDIS_ASIC_STATE_COMMAND_SET    "set"
#define REDIS_ASIC_STATE_COMMAND_GET    "get"

#define REDIS_ASIC_STATE_COMMAND_NOTIFY      "notify"

#define REDIS_ASIC_STATE_COMMAND_GET_STATS          "get_stats"
#define REDIS_ASIC_STATE_COMMAND_CLEAR_STATS        "clear_stats"

#define REDIS_ASIC_STATE_COMMAND_GETRESPONSE        "getresponse"

// TODO move this to OTAI meta repository for auto generate

#define OTAI_APS_NOTIFICATION_NAME_OLP_SWITCH_NOTIFY                 "olp_switch_notify"
#define OTAI_LINECARD_NOTIFICATION_NAME_LINECARD_STATE_CHANGE        "linecard_state_change"
#define OTAI_LINECARD_NOTIFICATION_NAME_LINECARD_ALARM_NOTIFY        "linecard_alarm_notify"
#define OTAI_OCM_NOTIFICATION_NAME_SPECTRUM_POWER_NOTIFY             "spectrum_power_notify"
#define OTAI_OTDR_NOTIFICATION_NAME_RESULT_NOTIFY                    "otdr_result_notify"

#define SYNCD_NOTIFICATION_CHANNEL_LINECARDSTATE "LINECARDSTATE"

/**
 * @brief Redis virtual object id counter key name.
 *
 * This key will be used by otairedis and syncd in REDIS database to generate
 * new object indexes used when constructing new virtual object id (VID).
 *
 * This key must have atomic access since it can be used at any time by syncd
 * process or orchagent process.
 */
#define REDIS_KEY_VIDCOUNTER "VIDCOUNTER"

/**
 * @brief Table which will be used to forward notifications from syncd.
 */
#define REDIS_TABLE_NOTIFICATIONS   "NOTIFICATIONS"

/**
 * @brief Table which will be used to send API response from syncd.
 */
#define REDIS_TABLE_GETRESPONSE     "GETRESPONSE"

// REDIS default database defines

#define REDIS_DEFAULT_DATABASE_ASIC         "ASIC_DB"
#define REDIS_DEFAULT_DATABASE_STATE        "STATE_DB"
#define REDIS_DEFAULT_DATABASE_COUNTERS     "COUNTERS_DB"

