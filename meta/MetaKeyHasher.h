#pragma once

extern "C" {
#include "laimetadata.h"
}

#include <string>

namespace laimeta
{
    struct MetaKeyHasher
    {
        std::size_t operator()(
                _In_ const lai_object_meta_key_t& k) const;

        bool operator()(
                _In_ const lai_object_meta_key_t& a,
                _In_ const lai_object_meta_key_t& b) const;
    };
}
