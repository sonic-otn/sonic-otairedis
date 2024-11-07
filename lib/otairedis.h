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
     * @brief Enable redis pipeline
     *
     * @type bool
     * @flags CREATE_AND_SET
     * @default false
     */
    OTAI_REDIS_LINECARD_ATTR_USE_PIPELINE = OTAI_LINECARD_ATTR_CUSTOM_RANGE_START,

    /**
     * @brief Will flush redis pipeline
     *
     * @type bool
     * @flags CREATE_AND_SET
     * @default false
     */
    OTAI_REDIS_LINECARD_ATTR_FLUSH,

    /**
     * @brief Redis communication mode.
     *
     * @type otai_redis_communication_mode_t
     * @flags CREATE_AND_SET
     * @default OTAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC
     */
    OTAI_REDIS_LINECARD_ATTR_REDIS_COMMUNICATION_MODE,

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
