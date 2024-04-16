#pragma once

#include "LinecardStateBase.h"

namespace otaivs
{
    class LinecardP230C:
        public LinecardStateBase
    {
        public:

            LinecardP230C(
                    _In_ otai_object_id_t linecard_id,
                    _In_ std::shared_ptr<RealObjectIdManager> manager,
                    _In_ std::shared_ptr<LinecardConfig> config);

            virtual ~LinecardP230C() = default;

    };
}
