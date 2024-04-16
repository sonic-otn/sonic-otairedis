#include "Globals.h"

#include "otai_serialize.h"

using namespace otaimeta;

std::string Globals::getAttrInfo(
        _In_ const otai_attr_metadata_t& md)
{
    SWSS_LOG_ENTER();

    /*
     * Attribute name will contain object type as well so we don't need to
     * serialize object type separately.
     */

    return std::string(md.attridname) + ":" + otai_serialize_attr_value_type(md.attrvaluetype);
}


