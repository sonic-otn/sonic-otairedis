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

typedef enum _otai_redis_linecard_attr_t
{
    /**
     * @brief Will flush redis pipeline
     *
     * @type bool
     * @flags CREATE_AND_SET
     * @default false
     */
    OTAI_REDIS_LINECARD_ATTR_FLUSH = OTAI_LINECARD_ATTR_CUSTOM_RANGE_START,

} otai_redis_linecard_attr_t;
