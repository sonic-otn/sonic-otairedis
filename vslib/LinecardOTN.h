#pragma once

#include "LinecardStateBase.h"

namespace otaivs
{
    class LinecardOTN:
        public LinecardStateBase
    {
        public:

            LinecardOTN(
                    _In_ otai_object_id_t linecard_id,
                    _In_ std::shared_ptr<RealObjectIdManager> manager,
                    _In_ std::shared_ptr<LinecardConfig> config);

            virtual ~LinecardOTN() = default;

    };
}
