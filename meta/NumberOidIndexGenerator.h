#pragma once

#include "lib/inc/OidIndexGenerator.h"

namespace laimeta
{
    class NumberOidIndexGenerator:
        public lairedis::OidIndexGenerator
    {

        public:

            NumberOidIndexGenerator();

            virtual ~NumberOidIndexGenerator() = default;

        public:

            virtual uint64_t increment() override;

            virtual void reset() override;

        private:

            uint64_t m_index;
    };
}
