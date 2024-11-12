#pragma once

extern "C" {
#include "otai.h"
}

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
