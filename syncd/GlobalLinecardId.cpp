#include "GlobalLinecardId.h"

#include "meta/lai_serialize.h"

#include "swss/logger.h"

using namespace syncd;

#ifdef LAITHRIFT

/**
 * @brief Global linecard RID used by thrift RPC server.
 */
lai_object_id_t gLinecardId = LAI_NULL_OBJECT_ID;

#endif

void GlobalLinecardId::setLinecardId(
        _In_ lai_object_id_t linecardRid)
{
    SWSS_LOG_ENTER();

#ifdef LAITHRIFT

    if (gLinecardId != LAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("gLinecardId already initialized!, LAI THRIFT don't support multiple linecards yet, FIXME");
    }

    gLinecardId = linecardRid;

    SWSS_LOG_NOTICE("Initialize gLinecardId with ID = %s",
            lai_serialize_object_id(gLinecardId).c_str());
#endif

}
