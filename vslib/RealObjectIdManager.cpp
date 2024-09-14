#include "RealObjectIdManager.h"

#include "meta/otai_serialize.h"
#include "swss/logger.h"

extern "C" {
#include "otaimetadata.h"
}

#include <inttypes.h>

#define OTAI_OBJECT_ID_BITS_SIZE (8 * sizeof(otai_object_id_t))

static_assert(OTAI_OBJECT_ID_BITS_SIZE == 64, "otai_object_id_t must have 64 bits");
static_assert(sizeof(otai_object_id_t) == sizeof(uint64_t), "OTAI object ID size should be uint64_t");

#define OTAI_VS_OID_RESERVED_BITS_SIZE ( 8 )

#define OTAI_VS_LINECARD_INDEX_BITS_SIZE ( 8 )
#define OTAI_VS_LINECARD_INDEX_MAX ( (1ULL << OTAI_VS_LINECARD_INDEX_BITS_SIZE) - 1 )
#define OTAI_VS_LINECARD_INDEX_MASK (OTAI_VS_LINECARD_INDEX_MAX)

#define OTAI_VS_GLOBAL_CONTEXT_BITS_SIZE ( 8 )
#define OTAI_VS_GLOBAL_CONTEXT_MAX ( (1ULL << OTAI_VS_GLOBAL_CONTEXT_BITS_SIZE) - 1 )
#define OTAI_VS_GLOBAL_CONTEXT_MASK (OTAI_VS_GLOBAL_CONTEXT_MAX)

#define OTAI_VS_OBJECT_TYPE_BITS_SIZE ( 8 )
#define OTAI_VS_OBJECT_TYPE_MAX ( (1ULL << OTAI_VS_OBJECT_TYPE_BITS_SIZE) - 1 )
#define OTAI_VS_OBJECT_TYPE_MASK (OTAI_VS_OBJECT_TYPE_MAX)

#define OTAI_VS_OBJECT_INDEX_BITS_SIZE ( 32 )
#define OTAI_VS_OBJECT_INDEX_MAX ( (1ULL << OTAI_VS_OBJECT_INDEX_BITS_SIZE) - 1 )
#define OTAI_VS_OBJECT_INDEX_MASK (OTAI_VS_OBJECT_INDEX_MAX)

#define OTAI_VS_OBJECT_ID_BITS_SIZE (      \
        OTAI_VS_OID_RESERVED_BITS_SIZE +   \
        OTAI_VS_GLOBAL_CONTEXT_BITS_SIZE + \
        OTAI_VS_LINECARD_INDEX_BITS_SIZE +   \
        OTAI_VS_OBJECT_TYPE_BITS_SIZE +    \
        OTAI_VS_OBJECT_INDEX_BITS_SIZE )

static_assert(OTAI_VS_OBJECT_ID_BITS_SIZE == OTAI_OBJECT_ID_BITS_SIZE, "vs object id size must be equal to OTAI object id size");

/*
 * This condition must be met, since we need to be able to encode OTAI object
 * type in object id on defined number of bits.
 */
static_assert(OTAI_OBJECT_TYPE_EXTENSIONS_MAX < OTAI_VS_OBJECT_TYPE_MAX, "vs max object type value must be greater than supported OTAI max objeect type value");

/*
 * Current OBJECT ID format:
 *
 * bits 63..56 - reserved (must be zero)
 * bits 55..48 - global context
 * bits 47..40 - linecard index
 * bits 49..32 - OTAI object type
 * bits 31..0  - object index
 *
 * So large number of bits is required, otherwise we would need to have map of
 * OID to some struct that will have all those values.  But having all this
 * information in OID itself is more convenient.
 */

#define OTAI_VS_GET_OBJECT_INDEX(oid) \
    ( ((uint64_t)oid) & ( OTAI_VS_OBJECT_INDEX_MASK ) )

#define OTAI_VS_GET_OBJECT_TYPE(oid) \
    ( (((uint64_t)oid) >> ( OTAI_VS_OBJECT_INDEX_BITS_SIZE) ) & ( OTAI_VS_OBJECT_TYPE_MASK ) )

#define OTAI_VS_GET_LINECARD_INDEX(oid) \
    ( (((uint64_t)oid) >> ( OTAI_VS_OBJECT_TYPE_BITS_SIZE + OTAI_VS_OBJECT_INDEX_BITS_SIZE) ) & ( OTAI_VS_LINECARD_INDEX_MASK ) )

#define OTAI_VS_GET_GLOBAL_CONTEXT(oid) \
    ( (((uint64_t)oid) >> ( OTAI_VS_LINECARD_INDEX_BITS_SIZE + OTAI_VS_OBJECT_TYPE_BITS_SIZE + OTAI_VS_OBJECT_INDEX_BITS_SIZE) ) & ( OTAI_VS_GLOBAL_CONTEXT_MASK ) )

#define OTAI_VS_TEST_OID (0x0123456789abcdef)

static_assert(OTAI_VS_GET_GLOBAL_CONTEXT(OTAI_VS_TEST_OID) == 0x23, "test global context");
static_assert(OTAI_VS_GET_LINECARD_INDEX(OTAI_VS_TEST_OID) == 0x45, "test linecard index");
static_assert(OTAI_VS_GET_OBJECT_TYPE(OTAI_VS_TEST_OID) == 0x67, "test object type");
static_assert(OTAI_VS_GET_OBJECT_INDEX(OTAI_VS_TEST_OID) == 0x89abcdef, "test object index");

using namespace otaivs;

uint64_t RealObjectIdManager::allocateNewObjectIndex(
        _In_ otai_object_type_t objectType,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    uint64_t objectIndex = 0;

    if (objectType == OTAI_OBJECT_TYPE_OCM)
    {
        for (uint32_t i = 0; i < attr_count; i++)
        {
            if (attr_list[i].id == OTAI_OCM_ATTR_ID)
            {
                objectIndex = attr_list[i].value.u32;
                break;
            }
        }        
    }
    else if (objectType == OTAI_OBJECT_TYPE_OTDR)
    {
        for (uint32_t i = 0; i < attr_count; i++)
        {
            if (attr_list[i].id == OTAI_OTDR_ATTR_ID)
            {
                objectIndex = attr_list[i].value.u32;
                break;
            }
        }
        
    }
    else
    {
        objectIndex = m_indexer[objectType]++;
    }

    return objectIndex;
}

otai_object_id_t RealObjectIdManager::allocateNewObjectId(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t linecardId,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if ((objectType <= OTAI_OBJECT_TYPE_NULL) || (objectType >= OTAI_OBJECT_TYPE_EXTENSIONS_MAX))
    {
        SWSS_LOG_THROW("invalid objct type: %d", objectType);
    }

    if (objectType == OTAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_THROW("this function can't be used to allocate linecard id");
    }

    otai_object_type_t linecardObjectType = objectTypeQuery(linecardId);

    if (linecardObjectType != OTAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_THROW("object type of linecard %s is %s, should be LINECARD",
                otai_serialize_object_id(linecardId).c_str(),
                otai_serialize_object_type(linecardObjectType).c_str());
    }

    uint32_t linecardIndex = (uint32_t)OTAI_VS_GET_LINECARD_INDEX(linecardId);

    // count from zero
    uint64_t objectIndex = allocateNewObjectIndex(objectType, attr_count, attr_list);

    if (objectIndex > OTAI_VS_OBJECT_INDEX_MAX)
    {
        SWSS_LOG_THROW("no more object indexes available, given: 0x%" PRIx64 " but limit is 0x%llx",
                objectIndex,
                OTAI_VS_OBJECT_INDEX_MAX);
    }

    otai_object_id_t objectId = constructObjectId(objectType, linecardIndex, objectIndex);

    SWSS_LOG_DEBUG("created RID %s",
            otai_serialize_object_id(objectId).c_str());

    return objectId;
}

otai_object_id_t RealObjectIdManager::allocateNewLinecardObjectId()
{
    SWSS_LOG_ENTER();

    uint32_t linecardIndex = 1;
    otai_object_id_t objectId = constructObjectId(OTAI_OBJECT_TYPE_LINECARD, linecardIndex, linecardIndex);
    SWSS_LOG_NOTICE("created LINECARD RID %s",
            otai_serialize_object_id(objectId).c_str());

    return objectId;
}

otai_object_id_t RealObjectIdManager::constructObjectId(
        _In_ otai_object_type_t objectType,
        _In_ uint32_t linecardIndex,
        _In_ uint64_t objectIndex,
        _In_ uint32_t globalContext)
{
    SWSS_LOG_ENTER();

    return (otai_object_id_t)(
            ((uint64_t)globalContext << ( OTAI_VS_LINECARD_INDEX_BITS_SIZE + OTAI_VS_OBJECT_TYPE_BITS_SIZE + OTAI_VS_OBJECT_INDEX_BITS_SIZE )) |
            ((uint64_t)linecardIndex << ( OTAI_VS_OBJECT_TYPE_BITS_SIZE + OTAI_VS_OBJECT_INDEX_BITS_SIZE )) |
            ((uint64_t)objectType << ( OTAI_VS_OBJECT_INDEX_BITS_SIZE)) |
            objectIndex);
}

otai_object_id_t RealObjectIdManager::linecardIdQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (objectId == OTAI_NULL_OBJECT_ID)
    {
        return OTAI_NULL_OBJECT_ID;
    }

    otai_object_type_t objectType = objectTypeQuery(objectId);

    if (objectType == OTAI_OBJECT_TYPE_NULL)
    {
        SWSS_LOG_ERROR("invalid object type of oid %s",
                otai_serialize_object_id(objectId).c_str());

        return OTAI_NULL_OBJECT_ID;
    }

    if (objectType == OTAI_OBJECT_TYPE_LINECARD)
    {
        return objectId;
    }

    uint32_t linecardIndex = (uint32_t)OTAI_VS_GET_LINECARD_INDEX(objectId);
    uint32_t globalContext = (uint32_t)OTAI_VS_GET_GLOBAL_CONTEXT(objectId);

    return constructObjectId(OTAI_OBJECT_TYPE_LINECARD, linecardIndex, linecardIndex, globalContext);
}

otai_object_type_t RealObjectIdManager::objectTypeQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (objectId == OTAI_NULL_OBJECT_ID)
    {
        return OTAI_OBJECT_TYPE_NULL;
    }

    otai_object_type_t objectType = (otai_object_type_t)(OTAI_VS_GET_OBJECT_TYPE(objectId));

    if (!otai_metadata_is_object_type_valid(objectType))
    {
        SWSS_LOG_ERROR("invalid object id %s",
                otai_serialize_object_id(objectId).c_str());

        return OTAI_OBJECT_TYPE_NULL;
    }

    return objectType;
}

uint32_t RealObjectIdManager::getLinecardIndex(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    auto linecardId = linecardIdQuery(objectId);

    return OTAI_VS_GET_LINECARD_INDEX(linecardId);
}

uint32_t RealObjectIdManager::getObjectIndex(
        _In_ otai_object_id_t objectId)
{
    return OTAI_VS_GET_OBJECT_INDEX(objectId);
}

