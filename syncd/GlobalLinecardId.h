#pragma once

extern "C" {
#include "lai.h"
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
                    _In_ lai_object_id_t linecardRid);
    };
}
