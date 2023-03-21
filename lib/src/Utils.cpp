#include "Utils.h"

extern "C" {
#include "laimetadata.h"
}

#include "meta/lai_serialize.h"
#include "swss/logger.h"

using namespace lairedis;

void Utils::clearOidList(
        _Out_ lai_object_list_t& list)
{
    SWSS_LOG_ENTER();

    for (uint32_t idx = 0; idx < list.count; ++idx)
    {
        list.list[idx] = LAI_NULL_OBJECT_ID;
    }
}

void Utils::clearOidValues(
        _In_ lai_object_type_t objectType,
        _In_ uint32_t attrCount,
        _Out_ lai_attribute_t *attrList)
{
    SWSS_LOG_ENTER();

    for (uint32_t i = 0; i < attrCount; i++)
    {
        lai_attribute_t &attr = attrList[i];

        auto meta = lai_metadata_get_attr_metadata(objectType, attr.id);

        if (meta == NULL)
        {
            SWSS_LOG_THROW("unable to get metadata for object type %s, attribute %d",
                    lai_serialize_object_type(objectType).c_str(),
                    attr.id);
        }

        switch (meta->attrvaluetype)
        {
            case LAI_ATTR_VALUE_TYPE_OBJECT_ID:
                attr.value.oid = LAI_NULL_OBJECT_ID;
                break;

            case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:
                clearOidList(attr.value.objlist);
                break;

            default:

                if (meta->isoidattribute)
                {
                    SWSS_LOG_THROW("attribute %s is object id, but not processed, FIXME", meta->attridname);
                }

                break;
        }

    }
}
