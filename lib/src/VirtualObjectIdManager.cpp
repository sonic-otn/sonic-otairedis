#include "VirtualObjectIdManager.h"

#include "meta/lai_serialize.h"
#include "swss/logger.h"
#include <inttypes.h>

extern "C" {
#include "laimetadata.h"
}

#define LAI_OBJECT_ID_BITS_SIZE (8 * sizeof(lai_object_id_t))

static_assert(LAI_OBJECT_ID_BITS_SIZE == 64, "lai_object_id_t must have 64 bits");
static_assert(sizeof(lai_object_id_t) == sizeof(uint64_t), "LAI object ID size should be uint64_t");

#define LAI_REDIS_LINECARD_INDEX_BITS_SIZE ( 8 )
#define LAI_REDIS_LINECARD_INDEX_MAX ( (1ULL << LAI_REDIS_LINECARD_INDEX_BITS_SIZE) - 1 )
#define LAI_REDIS_LINECARD_INDEX_MASK (LAI_REDIS_LINECARD_INDEX_MAX)

#define LAI_REDIS_GLOBAL_CONTEXT_BITS_SIZE ( 8 )
#define LAI_REDIS_GLOBAL_CONTEXT_MAX ( (1ULL << LAI_REDIS_GLOBAL_CONTEXT_BITS_SIZE) - 1 )
#define LAI_REDIS_GLOBAL_CONTEXT_MASK (LAI_REDIS_GLOBAL_CONTEXT_MAX)

#define LAI_REDIS_OBJECT_TYPE_BITS_SIZE ( 8 )
#define LAI_REDIS_OBJECT_TYPE_MAX ( (1ULL << LAI_REDIS_OBJECT_TYPE_BITS_SIZE) - 1 )
#define LAI_REDIS_OBJECT_TYPE_MASK (LAI_REDIS_OBJECT_TYPE_MAX)

#define LAI_REDIS_OBJECT_INDEX_BITS_SIZE ( 40 )
#define LAI_REDIS_OBJECT_INDEX_MAX ( (1ULL << LAI_REDIS_OBJECT_INDEX_BITS_SIZE) - 1 )
#define LAI_REDIS_OBJECT_INDEX_MASK (LAI_REDIS_OBJECT_INDEX_MAX)

#define LAI_REDIS_OBJECT_ID_BITS_SIZE (      \
        LAI_REDIS_LINECARD_INDEX_BITS_SIZE +   \
        LAI_REDIS_GLOBAL_CONTEXT_BITS_SIZE + \
        LAI_REDIS_OBJECT_TYPE_BITS_SIZE +    \
        LAI_REDIS_OBJECT_INDEX_BITS_SIZE )

static_assert(LAI_REDIS_OBJECT_ID_BITS_SIZE == LAI_OBJECT_ID_BITS_SIZE, "redis object id size must be equal to LAI object id size");

/*
 * This condition must be met, since we need to be able to encode LAI object
 * type in object id on defined number of bits.
 */
static_assert(LAI_OBJECT_TYPE_EXTENSIONS_MAX < LAI_REDIS_OBJECT_TYPE_MAX, "redis max object type value must be greater than supported LAI max object type value");

/*
 * Current OBJECT ID format:
 *
 * bits 63..56 - linecard index
 * bits 55..48 - LAI object type
 * bits 47..40 - global context
 * bits 40..0  - object index
 *
 * So large number of bits is required, otherwise we would need to have map of
 * OID to some struct that will have all those values.  But having all this
 * information in OID itself is more convenient.
 */

#define LAI_REDIS_GET_OBJECT_INDEX(oid) \
    ( ((uint64_t)oid) & ( LAI_REDIS_OBJECT_INDEX_MASK ) )

#define LAI_REDIS_GET_GLOBAL_CONTEXT(oid) \
    ( (((uint64_t)oid) >> (LAI_REDIS_OBJECT_INDEX_BITS_SIZE) ) & ( LAI_REDIS_GLOBAL_CONTEXT_MASK ) )

#define LAI_REDIS_GET_OBJECT_TYPE(oid) \
    ( (((uint64_t)oid) >> ( LAI_REDIS_GLOBAL_CONTEXT_BITS_SIZE + LAI_REDIS_OBJECT_INDEX_BITS_SIZE) ) & ( LAI_REDIS_OBJECT_TYPE_MASK ) )

#define LAI_REDIS_GET_LINECARD_INDEX(oid) \
    ( (((uint64_t)oid) >> ( LAI_REDIS_OBJECT_TYPE_BITS_SIZE + LAI_REDIS_GLOBAL_CONTEXT_BITS_SIZE + LAI_REDIS_OBJECT_INDEX_BITS_SIZE) ) & ( LAI_REDIS_LINECARD_INDEX_MASK ) )

#define LAI_REDIS_TEST_OID (0x0123456789abcdef)

static_assert(LAI_REDIS_GET_LINECARD_INDEX(LAI_REDIS_TEST_OID) == 0x01, "test linecard index");
static_assert(LAI_REDIS_GET_OBJECT_TYPE(LAI_REDIS_TEST_OID) == 0x23, "test object type");
static_assert(LAI_REDIS_GET_GLOBAL_CONTEXT(LAI_REDIS_TEST_OID) == 0x45, "test global context");
static_assert(LAI_REDIS_GET_OBJECT_INDEX(LAI_REDIS_TEST_OID) == 0x6789abcdef, "test object index");

using namespace lairedis;

VirtualObjectIdManager::VirtualObjectIdManager(
        _In_ uint32_t globalContext,
        _In_ std::shared_ptr<LinecardConfigContainer> scc,
        _In_ std::shared_ptr<OidIndexGenerator> oidIndexGenerator):
    m_globalContext(globalContext),
    m_container(scc),
    m_oidIndexGenerator(oidIndexGenerator)
{
    SWSS_LOG_ENTER();

    if (globalContext > LAI_REDIS_GLOBAL_CONTEXT_MAX)
    {
        SWSS_LOG_THROW("specified globalContext(0x%x) > maximum global context 0x%llx",
                globalContext,
                LAI_REDIS_GLOBAL_CONTEXT_MAX);
    }
}

lai_object_id_t VirtualObjectIdManager::laiLinecardIdQuery(
        _In_ lai_object_id_t objectId) const
{
    SWSS_LOG_ENTER();

    if (objectId == LAI_NULL_OBJECT_ID)
    {
        return LAI_NULL_OBJECT_ID;
    }

    lai_object_type_t objectType = laiObjectTypeQuery(objectId);

    if (objectType == LAI_OBJECT_TYPE_NULL)
    {
        // TODO don't throw, those 2 functions should never throw
        // it doesn't matter whether oid is correct, that will be validated
        // in metadata
        SWSS_LOG_THROW("invalid object type of oid %s",
                lai_serialize_object_id(objectId).c_str());
    }

    if (objectType == LAI_OBJECT_TYPE_LINECARD)
    {
        return objectId;
    }

    // NOTE: we could also check:
    // - if object id has correct global context
    // - if object id has existing linecard index
    // but then this method can't be made static

    uint32_t linecardIndex = (uint32_t)LAI_REDIS_GET_LINECARD_INDEX(objectId);

    uint32_t globalContext = (uint32_t)LAI_REDIS_GET_GLOBAL_CONTEXT(objectId);

    return constructObjectId(LAI_OBJECT_TYPE_LINECARD, linecardIndex, linecardIndex, globalContext);
}

lai_object_type_t VirtualObjectIdManager::laiObjectTypeQuery(
        _In_ lai_object_id_t objectId) const
{
    SWSS_LOG_ENTER();

    if (objectId == LAI_NULL_OBJECT_ID)
    {
        return LAI_OBJECT_TYPE_NULL;
    }

    lai_object_type_t objectType = (lai_object_type_t)(LAI_REDIS_GET_OBJECT_TYPE(objectId));

    if (objectType == LAI_OBJECT_TYPE_NULL || objectType >= LAI_OBJECT_TYPE_EXTENSIONS_MAX)
    {
        SWSS_LOG_ERROR("invalid object id %s",
                lai_serialize_object_id(objectId).c_str());

        /*
         * We can't throw here, since it would give no meaningful message.
         * Throwing at one level up is better.
         */

        return LAI_OBJECT_TYPE_NULL;
    }

    // NOTE: we could also check:
    // - if object id has correct global context
    // - if object id has existing linecard index
    // but then this method can't be made static

    return objectType;
}

void VirtualObjectIdManager::clear()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_NOTICE("clearing linecard index set");

    m_linecardIndexes.clear();
}

uint32_t VirtualObjectIdManager::allocateNewLinecardIndex()
{
    SWSS_LOG_ENTER();

    for (uint32_t index = 0; index < LAI_REDIS_LINECARD_INDEX_MAX; ++index)
    {
        if (m_linecardIndexes.find(index) != m_linecardIndexes.end())
            continue;

        m_linecardIndexes.insert(index);

        SWSS_LOG_NOTICE("allocated new linecard index 0x%x", index);

        return index;
    }

    SWSS_LOG_THROW("no more available linecard indexes (used count is: %zu)", m_linecardIndexes.size());
}

void VirtualObjectIdManager::releaseLinecardIndex(
        _In_ uint32_t index)
{
    SWSS_LOG_ENTER();

    auto it = m_linecardIndexes.find(index);

    if (it == m_linecardIndexes.end())
    {
        SWSS_LOG_THROW("linecard index 0x%x is invalid! programming error", index);
    }

    m_linecardIndexes.erase(it);

    SWSS_LOG_DEBUG("released linecard index 0x%x", index);
}


lai_object_id_t VirtualObjectIdManager::allocateNewObjectId(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t linecardId)
{
    SWSS_LOG_ENTER();

    if ((objectType <= LAI_OBJECT_TYPE_NULL) || (objectType >= LAI_OBJECT_TYPE_EXTENSIONS_MAX))
    {
        SWSS_LOG_THROW("invalid object type: %d", objectType);
    }

    if (objectType == LAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_THROW("this function can't be used to allocate linecard id");
    }

    lai_object_type_t linecardObjectType = laiObjectTypeQuery(linecardId);

    if (linecardObjectType != LAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_THROW("object type of linecard %s is %s, should be LINECARD",
                lai_serialize_object_id(linecardId).c_str(),
                lai_serialize_object_type(linecardObjectType).c_str());
    }

    uint32_t linecardIndex = (uint32_t)LAI_REDIS_GET_LINECARD_INDEX(linecardId);

    uint64_t objectIndex = m_oidIndexGenerator->increment(); // get new object index

    if (objectIndex > LAI_REDIS_OBJECT_INDEX_MAX)
    {
        SWSS_LOG_THROW("no more object indexes available, given: 0x%" PRIu64 " but limit is 0x%llu",
                objectIndex,
                LAI_REDIS_OBJECT_INDEX_MAX);
    }

    lai_object_id_t objectId = constructObjectId(objectType, linecardIndex, objectIndex, m_globalContext);

    SWSS_LOG_DEBUG("created VID %s",
            lai_serialize_object_id(objectId).c_str());

    return objectId;
}

lai_object_id_t VirtualObjectIdManager::allocateNewLinecardObjectId(
        _In_ const std::string& hardwareInfo)
{
    SWSS_LOG_ENTER();

    auto config = m_container->getConfig(hardwareInfo);

    if (config == nullptr)
    {
        SWSS_LOG_ERROR("no linecard config for hardware info: '%s'", hardwareInfo.c_str());

        return LAI_NULL_OBJECT_ID;
    }

    uint32_t linecardIndex = config->m_linecardIndex;

    if (linecardIndex > LAI_REDIS_LINECARD_INDEX_MAX)
    {
        SWSS_LOG_THROW("linecard index %u > %llu (max)", linecardIndex, LAI_REDIS_LINECARD_INDEX_MAX);
    }

    m_linecardIndexes.insert(linecardIndex);

    lai_object_id_t objectId = constructObjectId(LAI_OBJECT_TYPE_LINECARD, linecardIndex, linecardIndex, m_globalContext);

    SWSS_LOG_NOTICE("created LINECARD VID %s for hwinfo: '%s'",
            lai_serialize_object_id(objectId).c_str(),
            hardwareInfo.c_str());

    return objectId;
}


void VirtualObjectIdManager::releaseObjectId(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (laiObjectTypeQuery(objectId) == LAI_OBJECT_TYPE_LINECARD)
    {
        releaseLinecardIndex((uint32_t)LAI_REDIS_GET_LINECARD_INDEX(objectId));
    }
}

lai_object_id_t VirtualObjectIdManager::constructObjectId(
        _In_ lai_object_type_t objectType,
        _In_ uint32_t linecardIndex,
        _In_ uint64_t objectIndex,
        _In_ uint32_t globalContext)
{
    SWSS_LOG_ENTER();

    return (lai_object_id_t)(
            ((uint64_t)linecardIndex << (LAI_REDIS_OBJECT_TYPE_BITS_SIZE + LAI_REDIS_GLOBAL_CONTEXT_BITS_SIZE + LAI_REDIS_OBJECT_INDEX_BITS_SIZE)) |
            ((uint64_t)objectType << (LAI_REDIS_GLOBAL_CONTEXT_BITS_SIZE + LAI_REDIS_OBJECT_INDEX_BITS_SIZE)) |
            ((uint64_t)globalContext << (LAI_REDIS_OBJECT_INDEX_BITS_SIZE)) |
            objectIndex);
}

lai_object_id_t VirtualObjectIdManager::linecardIdQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (objectId == LAI_NULL_OBJECT_ID)
    {
        return LAI_NULL_OBJECT_ID;
    }

    lai_object_type_t objectType = objectTypeQuery(objectId);

    if (objectType == LAI_OBJECT_TYPE_NULL)
    {
        SWSS_LOG_ERROR("invalid object type of oid %s",
                lai_serialize_object_id(objectId).c_str());

        return LAI_NULL_OBJECT_ID;
    }

    if (objectType == LAI_OBJECT_TYPE_LINECARD)
    {
        return objectId;
    }

    uint32_t linecardIndex = (uint32_t)LAI_REDIS_GET_LINECARD_INDEX(objectId);
    uint32_t globalContext = (uint32_t)LAI_REDIS_GET_GLOBAL_CONTEXT(objectId);

    return constructObjectId(LAI_OBJECT_TYPE_LINECARD, linecardIndex, linecardIndex, globalContext);
}

lai_object_type_t VirtualObjectIdManager::objectTypeQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (objectId == LAI_NULL_OBJECT_ID)
    {
        return LAI_OBJECT_TYPE_NULL;
    }

    lai_object_type_t objectType = (lai_object_type_t)(LAI_REDIS_GET_OBJECT_TYPE(objectId));

    if (!lai_metadata_is_object_type_valid(objectType))
    {
        SWSS_LOG_ERROR("invalid object id %s",
                lai_serialize_object_id(objectId).c_str());

        return LAI_OBJECT_TYPE_NULL;
    }

    return objectType;
}

uint32_t VirtualObjectIdManager::getLinecardIndex(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    auto linecardId = linecardIdQuery(objectId);

    return (uint32_t)LAI_REDIS_GET_LINECARD_INDEX(linecardId);
}

uint32_t VirtualObjectIdManager::getGlobalContext(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    auto linecardId = linecardIdQuery(objectId);

    return (uint32_t)LAI_REDIS_GET_GLOBAL_CONTEXT(linecardId);
}

uint64_t VirtualObjectIdManager::getObjectIndex(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return (uint32_t)LAI_REDIS_GET_OBJECT_INDEX(objectId);
}

lai_object_id_t VirtualObjectIdManager::updateObjectIndex(
        _In_ lai_object_id_t objectId,
        _In_ uint64_t objectIndex)
{
    SWSS_LOG_ENTER();

    if (objectId == LAI_NULL_OBJECT_ID)
    {
        SWSS_LOG_THROW("can't update object index on NULL_OBJECT_ID");
    }

    if (objectIndex > LAI_REDIS_OBJECT_INDEX_MAX)
    {
        SWSS_LOG_THROW("object index %" PRIu64 " over maximum %llu", objectIndex, LAI_REDIS_OBJECT_INDEX_MAX);
    }

    lai_object_type_t objectType = objectTypeQuery(objectId);

    if (objectType == LAI_OBJECT_TYPE_NULL)
    {
        SWSS_LOG_THROW("invalid object type of oid %s",
                lai_serialize_object_id(objectId).c_str());
    }

    uint32_t linecardIndex = (uint32_t)LAI_REDIS_GET_LINECARD_INDEX(objectId);
    uint32_t globalContext = (uint32_t)LAI_REDIS_GET_GLOBAL_CONTEXT(objectId);

    return constructObjectId(objectType, linecardIndex, objectIndex, globalContext);
}
