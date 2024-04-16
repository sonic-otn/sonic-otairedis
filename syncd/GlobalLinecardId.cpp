#include "GlobalLinecardId.h"

#include "meta/otai_serialize.h"

#include "swss/logger.h"

using namespace syncd;

#ifdef OTAITHRIFT

/**
 * @brief Global linecard RID used by thrift RPC server.
 */
otai_object_id_t gLinecardId = OTAI_NULL_OBJECT_ID;

#endif

void GlobalLinecardId::setLinecardId(
        _In_ otai_object_id_t linecardRid)
{
    SWSS_LOG_ENTER();

#ifdef OTAITHRIFT

    if (gLinecardId != OTAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("gLinecardId already initialized!, OTAI THRIFT don't support multiple linecards yet, FIXME");
    }

    gLinecardId = linecardRid;

    SWSS_LOG_NOTICE("Initialize gLinecardId with ID = %s",
            otai_serialize_object_id(gLinecardId).c_str());
#endif

}
