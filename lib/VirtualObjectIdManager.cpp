#include "VirtualObjectIdManager.h"

#include "meta/otai_serialize.h"
#include "swss/logger.h"
#include <inttypes.h>

extern "C" {
#include "otaimetadata.h"
}

#define OTAI_OBJECT_ID_BITS_SIZE (8 * sizeof(otai_object_id_t))

static_assert(OTAI_OBJECT_ID_BITS_SIZE == 64, "otai_object_id_t must have 64 bits");
static_assert(sizeof(otai_object_id_t) == sizeof(uint64_t), "OTAI object ID size should be uint64_t");

#define OTAI_REDIS_LINECARD_INDEX_BITS_SIZE ( 8 )
#define OTAI_REDIS_LINECARD_INDEX_MAX ( (1ULL << OTAI_REDIS_LINECARD_INDEX_BITS_SIZE) - 1 )
#define OTAI_REDIS_LINECARD_INDEX_MASK (OTAI_REDIS_LINECARD_INDEX_MAX)

#define OTAI_REDIS_OBJECT_TYPE_BITS_SIZE ( 8 )
#define OTAI_REDIS_OBJECT_TYPE_MAX ( (1ULL << OTAI_REDIS_OBJECT_TYPE_BITS_SIZE) - 1 )
#define OTAI_REDIS_OBJECT_TYPE_MASK (OTAI_REDIS_OBJECT_TYPE_MAX)

#define OTAI_REDIS_OBJECT_INDEX_BITS_SIZE ( 48 )
#define OTAI_REDIS_OBJECT_INDEX_MAX ( (1ULL << OTAI_REDIS_OBJECT_INDEX_BITS_SIZE) - 1 )
#define OTAI_REDIS_OBJECT_INDEX_MASK (OTAI_REDIS_OBJECT_INDEX_MAX)

#define OTAI_REDIS_OBJECT_ID_BITS_SIZE (      \
        OTAI_REDIS_LINECARD_INDEX_BITS_SIZE +   \
        OTAI_REDIS_OBJECT_TYPE_BITS_SIZE +    \
        OTAI_REDIS_OBJECT_INDEX_BITS_SIZE )

static_assert(OTAI_REDIS_OBJECT_ID_BITS_SIZE == OTAI_OBJECT_ID_BITS_SIZE, "redis object id size must be equal to OTAI object id size");

/*
 * This condition must be met, since we need to be able to encode OTAI object
 * type in object id on defined number of bits.
 */
static_assert(OTAI_OBJECT_TYPE_EXTENSIONS_MAX < OTAI_REDIS_OBJECT_TYPE_MAX, "redis max object type value must be greater than supported OTAI max object type value");

/*
 * Current OBJECT ID format:
 *
 * bits 63..56 - linecard index
 * bits 55..48 - OTAI object type
 * bits 40..0  - object index
 *
 * So large number of bits is required, otherwise we would need to have map of
 * OID to some struct that will have all those values.  But having all this
 * information in OID itself is more convenient.
 */

#define OTAI_REDIS_GET_OBJECT_INDEX(oid) \
    ( ((uint64_t)oid) & ( OTAI_REDIS_OBJECT_INDEX_MASK ) )

#define OTAI_REDIS_GET_OBJECT_TYPE(oid) \
    ( (((uint64_t)oid) >> ( OTAI_REDIS_OBJECT_INDEX_BITS_SIZE) ) & ( OTAI_REDIS_OBJECT_TYPE_MASK ) )

#define OTAI_REDIS_GET_LINECARD_INDEX(oid) \
    ( (((uint64_t)oid) >> ( OTAI_REDIS_OBJECT_TYPE_BITS_SIZE + OTAI_REDIS_OBJECT_INDEX_BITS_SIZE) ) & ( OTAI_REDIS_LINECARD_INDEX_MASK ) )

#define OTAI_REDIS_TEST_OID (0x0123456789abcdef)

static_assert(OTAI_REDIS_GET_LINECARD_INDEX(OTAI_REDIS_TEST_OID) == 0x01, "test linecard index");
static_assert(OTAI_REDIS_GET_OBJECT_TYPE(OTAI_REDIS_TEST_OID) == 0x23, "test object type");
static_assert(OTAI_REDIS_GET_OBJECT_INDEX(OTAI_REDIS_TEST_OID) == 0x456789abcdef, "test object index");

using namespace otairedis;

VirtualObjectIdManager::VirtualObjectIdManager(
        _In_ std::shared_ptr<OidIndexGenerator> oidIndexGenerator):
    m_oidIndexGenerator(oidIndexGenerator)
{
    SWSS_LOG_ENTER();
}

otai_object_id_t VirtualObjectIdManager::otaiLinecardIdQuery(
        _In_ otai_object_id_t objectId) const
{
    SWSS_LOG_ENTER();

    if (objectId == OTAI_NULL_OBJECT_ID)
    {
        return OTAI_NULL_OBJECT_ID;
    }

    otai_object_type_t objectType = otaiObjectTypeQuery(objectId);

    if (objectType == OTAI_OBJECT_TYPE_NULL)
    {
        // TODO don't throw, those 2 functions should never throw
        // it doesn't matter whether oid is correct, that will be validated
        // in metadata
        SWSS_LOG_THROW("invalid object type of oid %s",
                otai_serialize_object_id(objectId).c_str());
    }

    if (objectType == OTAI_OBJECT_TYPE_LINECARD)
    {
        return objectId;
    }

    // NOTE: we could also check:
    // - if object id has existing linecard index
    // but then this method can't be made static

    uint32_t linecardIndex = (uint32_t)OTAI_REDIS_GET_LINECARD_INDEX(objectId);

    return constructObjectId(OTAI_OBJECT_TYPE_LINECARD, linecardIndex, linecardIndex);
}

otai_object_type_t VirtualObjectIdManager::otaiObjectTypeQuery(
        _In_ otai_object_id_t objectId) const
{
    SWSS_LOG_ENTER();

    if (objectId == OTAI_NULL_OBJECT_ID)
    {
        return OTAI_OBJECT_TYPE_NULL;
    }

    otai_object_type_t objectType = (otai_object_type_t)(OTAI_REDIS_GET_OBJECT_TYPE(objectId));

    if (objectType == OTAI_OBJECT_TYPE_NULL || objectType >= OTAI_OBJECT_TYPE_EXTENSIONS_MAX)
    {
        SWSS_LOG_ERROR("invalid object id %s",
                otai_serialize_object_id(objectId).c_str());

        /*
         * We can't throw here, since it would give no meaningful message.
         * Throwing at one level up is better.
         */

        return OTAI_OBJECT_TYPE_NULL;
    }

    // NOTE: we could also check:
    // - if object id has existing linecard index
    // but then this method can't be made static

    return objectType;
}

void VirtualObjectIdManager::clear()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("clearing linecard index set");

}


otai_object_id_t VirtualObjectIdManager::allocateNewObjectId(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t linecardId)
{
    SWSS_LOG_ENTER();

    if ((objectType <= OTAI_OBJECT_TYPE_NULL) || (objectType >= OTAI_OBJECT_TYPE_EXTENSIONS_MAX))
    {
        SWSS_LOG_THROW("invalid object type: %d", objectType);
    }

    if (objectType == OTAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_THROW("this function can't be used to allocate linecard id");
    }

    otai_object_type_t linecardObjectType = otaiObjectTypeQuery(linecardId);

    if (linecardObjectType != OTAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_THROW("object type of linecard %s is %s, should be LINECARD",
                otai_serialize_object_id(linecardId).c_str(),
                otai_serialize_object_type(linecardObjectType).c_str());
    }

    uint32_t linecardIndex = (uint32_t)OTAI_REDIS_GET_LINECARD_INDEX(linecardId);

    uint64_t objectIndex = m_oidIndexGenerator->increment(); // get new object index

    if (objectIndex > OTAI_REDIS_OBJECT_INDEX_MAX)
    {
        SWSS_LOG_THROW("no more object indexes available, given: 0x%" PRIu64 " but limit is 0x%llu",
                objectIndex,
                OTAI_REDIS_OBJECT_INDEX_MAX);
    }

    otai_object_id_t objectId = constructObjectId(objectType, linecardIndex, objectIndex);

    SWSS_LOG_DEBUG("created VID %s",
            otai_serialize_object_id(objectId).c_str());

    return objectId;
}

otai_object_id_t VirtualObjectIdManager::allocateNewLinecardObjectId()
{
    SWSS_LOG_ENTER();

    uint32_t linecardIndex = 0;
    otai_object_id_t objectId = constructObjectId(OTAI_OBJECT_TYPE_LINECARD, linecardIndex, linecardIndex);

    SWSS_LOG_NOTICE("created LINECARD VID %s for ",
            otai_serialize_object_id(objectId).c_str());

    return objectId;
}

otai_object_id_t VirtualObjectIdManager::constructObjectId(
        _In_ otai_object_type_t objectType,
        _In_ uint32_t linecardIndex,
        _In_ uint64_t objectIndex)
{
    SWSS_LOG_ENTER();

    return (otai_object_id_t)(
            ((uint64_t)linecardIndex << (OTAI_REDIS_OBJECT_TYPE_BITS_SIZE + OTAI_REDIS_OBJECT_INDEX_BITS_SIZE)) |
            ((uint64_t)objectType << OTAI_REDIS_OBJECT_INDEX_BITS_SIZE) |
            objectIndex);
}

otai_object_id_t VirtualObjectIdManager::linecardIdQuery(
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

    uint32_t linecardIndex = (uint32_t)OTAI_REDIS_GET_LINECARD_INDEX(objectId);

    return constructObjectId(OTAI_OBJECT_TYPE_LINECARD, linecardIndex, linecardIndex);
}

otai_object_type_t VirtualObjectIdManager::objectTypeQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (objectId == OTAI_NULL_OBJECT_ID)
    {
        return OTAI_OBJECT_TYPE_NULL;
    }

    otai_object_type_t objectType = (otai_object_type_t)(OTAI_REDIS_GET_OBJECT_TYPE(objectId));

    if (!otai_metadata_is_object_type_valid(objectType))
    {
        SWSS_LOG_ERROR("invalid object id %s",
                otai_serialize_object_id(objectId).c_str());

        return OTAI_OBJECT_TYPE_NULL;
    }

    return objectType;
}

uint64_t VirtualObjectIdManager::getObjectIndex(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return (uint32_t)OTAI_REDIS_GET_OBJECT_INDEX(objectId);
}
