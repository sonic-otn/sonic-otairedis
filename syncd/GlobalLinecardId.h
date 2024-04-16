#pragma once

extern "C" {
#include "otai.h"
}

namespace syncd
{
    class GlobalLinecardId
    {
        private:

            GlobalLinecardId() = delete;
            ~GlobalLinecardId() = delete;

        public:

            static void setLinecardId(
                    _In_ otai_object_id_t linecardRid);
    };
}
