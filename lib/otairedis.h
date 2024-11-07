#pragma once

extern "C" {
#include "otai.h"
}

/**
 * @brief Redis key context config.
 *
 * Optional. Should point to a context_config.json which will contain how many
 * contexts (syncd) we have in the system globally and each context how many
 * linecards it manages.
 */
#define OTAI_REDIS_KEY_CONTEXT_CONFIG              "OTAI_REDIS_CONTEXT_CONFIG"

/**
 * @brief Default synchronous operation response timeout in milliseconds.
 */
#define OTAI_REDIS_DEFAULT_SYNC_OPERATION_RESPONSE_TIMEOUT (17*1000)

typedef enum _otai_redis_notify_syncd_t
{
    OTAI_REDIS_NOTIFY_SYNCD_INIT_VIEW,

    OTAI_REDIS_NOTIFY_SYNCD_APPLY_VIEW,

    OTAI_REDIS_NOTIFY_SYNCD_INSPECT_ASIC

} otai_redis_notify_syncd_t;

typedef enum _otai_redis_communication_mode_t
{
    /**
     * @brief Asynchronous mode using Redis DB.
     */
    OTAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC,

    /**
     * @brief Synchronous mode using Redis DB.
     */
    OTAI_REDIS_COMMUNICATION_MODE_REDIS_SYNC,

} otai_redis_communication_mode_t;

typedef enum _otai_redis_linecard_attr_t
{
    /**
     * @brief Will notify syncd whether to init or apply view
     *
     * @type otai_redis_notify_syncd_t
     * @flags CREATE_AND_SET
     * @default OTAI_REDIS_NOTIFY_SYNCD_APPLY_VIEW
     */
    OTAI_REDIS_LINECARD_ATTR_NOTIFY_SYNCD = OTAI_LINECARD_ATTR_CUSTOM_RANGE_START,

    /**
     * @brief Use temporary view for all actions between
     * init and apply view. By default init and apply view will
     * not take effect. This is temporary solution until
     * comparison logic will be in place.
     *
     * @type bool
     * @flags CREATE_AND_SET
     * @default false
     */
    OTAI_REDIS_LINECARD_ATTR_USE_TEMP_VIEW,

    /**
     * @brief Enable redis pipeline
     *
     * @type bool
     * @flags CREATE_AND_SET
     * @default false
     */
    OTAI_REDIS_LINECARD_ATTR_USE_PIPELINE,

    /**
     * @brief Will flush redis pipeline
     *
     * @type bool
     * @flags CREATE_AND_SET
     * @default false
     */
    OTAI_REDIS_LINECARD_ATTR_FLUSH,

    /**
     * @brief Synchronous mode.
     *
     * Enable or disable synchronous mode. When enabled syncd also needs to be
     * running in synchronous mode. Command pipeline will be disabled when this
     * flag will be set to true.
     *
     * NOTE: This attribute is deprecated by
     * OTAI_REDIS_LINECARD_ATTR_REDIS_COMMUNICATION_MODE.  When set to true it
     * will set OTAI_REDIS_LINECARD_ATTR_REDIS_COMMUNICATION_MODE to
     * OTAI_REDIS_COMMUNICATION_MODE_REDIS_SYNC.
     *
     * TODO: remove this attribute.
     *
     * @type bool
     * @flags CREATE_AND_SET
     * @default false
     */
    OTAI_REDIS_LINECARD_ATTR_SYNC_MODE,

    /**
     * @brief Redis communication mode.
     *
     * @type otai_redis_communication_mode_t
     * @flags CREATE_AND_SET
     * @default OTAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC
     */
    OTAI_REDIS_LINECARD_ATTR_REDIS_COMMUNICATION_MODE,

    /**
     * @brief Global context.
     *
     * When creating linecard, this attribute can be specified (and must be
     * passed as last attribute on the list), will determine which context to
     * talk to.  Context is a syncd instance. Also this value is encoded
     * internally into each object ID, so each API call will know internally to
     * which instance of syncd send API requests.
     *
     * @type uint32_t
     * @flags CREATE_ONLY
     * @default 0
     */
    OTAI_REDIS_LINECARD_ATTR_CONTEXT,

    /**
     * @brief Synchronous operation response timeout in milliseconds.
     *
     * Used for every synchronous API call. In asynchronous mode used for GET
     * operation.
     *
     * @type otai_uint64_t
     * @flags CREATE_AND_SET
     * @default 60000
     */
    OTAI_REDIS_LINECARD_ATTR_SYNC_OPERATION_RESPONSE_TIMEOUT,

} otai_redis_linecard_attr_t;
