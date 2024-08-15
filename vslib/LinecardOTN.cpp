#include "LinecardOTN.h"

#include "swss/logger.h"
#include "meta/otai_serialize.h"

using namespace otaivs;

LinecardOTN::LinecardOTN(
        _In_ otai_object_id_t linecard_id,
        _In_ std::shared_ptr<RealObjectIdManager> manager,
        _In_ std::shared_ptr<LinecardConfig> config):
    LinecardStateBase(linecard_id, manager, config)
{
    SWSS_LOG_ENTER();

    // empty
}

