#include "MetaKeyHasher.h"
#include "lai_serialize.h"

#include "swss/logger.h"

#include <cstring>

using namespace laimeta;

bool MetaKeyHasher::operator()(
        _In_ const lai_object_meta_key_t& a,
        _In_ const lai_object_meta_key_t& b) const
{
    // SWSS_LOG_ENTER(); // disabled for performance reasons

    if (a.objecttype != b.objecttype)
        return false;

    auto meta = lai_metadata_get_object_type_info(a.objecttype);

    if (meta && meta->isobjectid)
        return a.objectkey.key.object_id == b.objectkey.key.object_id;

    SWSS_LOG_THROW("not implemented: %s",
            lai_serialize_object_meta_key(a).c_str());
}

static_assert(sizeof(std::size_t) >= sizeof(uint32_t), "size_t must be at least 32 bits");

static_assert(sizeof(uint32_t) == 4, "uint32_t expected to be 4 bytes");

std::size_t MetaKeyHasher::operator()(
        _In_ const lai_object_meta_key_t& k) const
{
    // SWSS_LOG_ENTER(); // disabled for performance reasons

    auto meta = lai_metadata_get_object_type_info(k.objecttype);

    if (meta && meta->isobjectid)
    {
        // cast is required in case size_t is 4 bytes (arm)
        return (std::size_t)k.objectkey.key.object_id;
    }

    SWSS_LOG_THROW("not handled: %s", lai_serialize_object_type(k.objecttype).c_str());
}
