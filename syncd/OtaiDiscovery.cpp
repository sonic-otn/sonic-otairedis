#include "OtaiDiscovery.h"

#include "swss/logger.h"

#include "meta/otai_serialize.h"

using namespace syncd;

/**
 * @def OTAI_DISCOVERY_LIST_MAX_ELEMENTS
 *
 * Defines maximum elements that can be obtained from the OID list when
 * performing list attribute query (discovery) on the linecard.
 *
 * This value will be used to allocate memory on the stack for obtaining object
 * list, and should be big enough to obtain list for all ports on the linecard
 * and vlan members.
 */
#define OTAI_DISCOVERY_LIST_MAX_ELEMENTS 1024

OtaiDiscovery::OtaiDiscovery(
    _In_ std::shared_ptr<otairedis::OtaiInterface> otai) :
    m_otai(otai)
{
    SWSS_LOG_ENTER();

    // empty
}

OtaiDiscovery::~OtaiDiscovery()
{
    SWSS_LOG_ENTER();

    // empty
}

void OtaiDiscovery::discover(
    _In_ otai_object_id_t rid,
    _Inout_ std::set<otai_object_id_t>& discovered)
{
    SWSS_LOG_ENTER();

    /*
     * NOTE: This method is only good after linecard init since we are making
     * assumptions that there are no ACL after initialization.
     *
     * NOTE: Input set could be a map of sets, this way we will also have
     * dependency on each oid.
     */

    if (rid == OTAI_NULL_OBJECT_ID)
    {
        return;
    }

    if (discovered.find(rid) != discovered.end())
    {
        return;
    }

    otai_object_type_t ot = m_otai->objectTypeQuery(rid);

    if (ot == OTAI_OBJECT_TYPE_NULL)
    {
        SWSS_LOG_THROW("objectTypeQuery: rid %s returned NULL object type",
            otai_serialize_object_id(rid).c_str());
    }

    SWSS_LOG_DEBUG("processing %s: %s",
        otai_serialize_object_id(rid).c_str(),
        otai_serialize_object_type(ot).c_str());

    discovered.insert(rid);

    const otai_object_type_info_t* info = otai_metadata_get_object_type_info(ot);

    /*
     * We will query only oid object types
     * then we don't need meta key, but we need to add to metadata
     * pointers to only generic functions.
     */

    otai_object_meta_key_t mk = { .objecttype = ot, .objectkey = {.key = {.object_id = rid } } };

    for (int idx = 0; info->attrmetadata[idx] != NULL; ++idx)
    {
        const otai_attr_metadata_t* md = info->attrmetadata[idx];

        /*
         * Note that we don't care about ACL object id's since
         * we assume that there are no ACLs on linecard after init.
         */

        otai_attribute_t attr;
        memset(&attr, 0, sizeof(attr));
        attr.id = md->attrid;

        if (md->attrvaluetype == OTAI_ATTR_VALUE_TYPE_OBJECT_ID)
        {
            if (md->defaultvaluetype == OTAI_DEFAULT_VALUE_TYPE_CONST)
            {
                /*
                 * This means that default value for this object is
                 * OTAI_NULL_OBJECT_ID, since this is discovery after
                 * create, we don't need to query this attribute.
                 */

                 //continue;
            }

            SWSS_LOG_DEBUG("getting %s for %s", md->attridname,
                otai_serialize_object_id(rid).c_str());

            otai_status_t status = m_otai->get(mk.objecttype, mk.objectkey.key.object_id, 1, &attr);

            if (status != OTAI_STATUS_SUCCESS)
            {
                /*
                 * We failed to get value, maybe it's not supported ?
                 */

                SWSS_LOG_INFO("%s: %s on %s",
                    md->attridname,
                    otai_serialize_status(status).c_str(),
                    otai_serialize_object_id(rid).c_str());

                continue;
            }

            m_defaultOidMap[rid][attr.id] = attr.value.oid;

            if (!md->allownullobjectid && attr.value.oid == OTAI_NULL_OBJECT_ID)
            {
                // SWSS_LOG_WARN("got null on %s, but not allowed", md->attridname);
            }

            if (attr.value.oid != OTAI_NULL_OBJECT_ID)
            {
                ot = m_otai->objectTypeQuery(attr.value.oid);

                if (ot == OTAI_OBJECT_TYPE_NULL)
                {
                    SWSS_LOG_THROW("when query %s (on %s RID %s) got value %s objectTypeQuery returned NULL object type",
                        md->attridname,
                        otai_serialize_object_type(md->objecttype).c_str(),
                        otai_serialize_object_id(rid).c_str(),
                        otai_serialize_object_id(attr.value.oid).c_str());
                }
            }

            discover(attr.value.oid, discovered); // recursion
        }
        else if (md->attrvaluetype == OTAI_ATTR_VALUE_TYPE_OBJECT_LIST)
        {
            if (md->defaultvaluetype == OTAI_DEFAULT_VALUE_TYPE_EMPTY_LIST)
            {
                /*
                 * This means that default value for this object is
                 * empty list, since this is discovery after
                 * create, we don't need to query this attribute.
                 */

                 //continue;
            }

            SWSS_LOG_DEBUG("getting %s for %s", md->attridname,
                otai_serialize_object_id(rid).c_str());

            otai_object_id_t local[OTAI_DISCOVERY_LIST_MAX_ELEMENTS];

            attr.value.objlist.count = OTAI_DISCOVERY_LIST_MAX_ELEMENTS;
            attr.value.objlist.list = local;

            otai_status_t status = m_otai->get(mk.objecttype, mk.objectkey.key.object_id, 1, &attr);

            if (status != OTAI_STATUS_SUCCESS)
            {
                /*
                 * We failed to get value, maybe it's not supported ?
                 */

                SWSS_LOG_INFO("%s: %s on %s",
                    md->attridname,
                    otai_serialize_status(status).c_str(),
                    otai_serialize_object_id(rid).c_str());

                continue;
            }

            SWSS_LOG_DEBUG("list count %s %u", md->attridname, attr.value.objlist.count);

            for (uint32_t i = 0; i < attr.value.objlist.count; ++i)
            {
                otai_object_id_t oid = attr.value.objlist.list[i];

                ot = m_otai->objectTypeQuery(oid);

                if (ot == OTAI_OBJECT_TYPE_NULL)
                {
                    SWSS_LOG_THROW("when query %s (on %s RID %s) got value %s objectTypeQuery returned NULL object type",
                        md->attridname,
                        otai_serialize_object_type(md->objecttype).c_str(),
                        otai_serialize_object_id(rid).c_str(),
                        otai_serialize_object_id(oid).c_str());
                }

                discover(oid, discovered); // recursion
            }
        }
    }
}

std::set<otai_object_id_t> OtaiDiscovery::discover(
    _In_ otai_object_id_t startRid)
{
    SWSS_LOG_ENTER();

    /*
     * Preform discovery on the linecard to obtain ASIC view of
     * objects that are created internally.
     */

    m_defaultOidMap.clear();

    std::set<otai_object_id_t> discovered_rids;

    {
        SWSS_LOG_TIMER("discover");

        //setApiLogLevel(OTAI_LOG_LEVEL_CRITICAL);

        discover(startRid, discovered_rids);

        //setApiLogLevel(OTAI_LOG_LEVEL_NOTICE);
    }

    SWSS_LOG_NOTICE("discovered objects count: %zu", discovered_rids.size());

    std::map<otai_object_type_t, int> map;

    for (otai_object_id_t rid : discovered_rids)
    {
        /*
         * We don't need to check for null since otaiDiscovery already checked
         * that.
         */

        map[m_otai->objectTypeQuery(rid)]++;
    }

    for (const auto& p : map)
    {
        SWSS_LOG_NOTICE("%s: %d", otai_serialize_object_type(p.first).c_str(), p.second);
    }

    return discovered_rids;
}

const OtaiDiscovery::DefaultOidMap& OtaiDiscovery::getDefaultOidMap() const
{
    SWSS_LOG_ENTER();

    return m_defaultOidMap;
}

void OtaiDiscovery::setApiLogLevel(
    _In_ otai_log_level_t logLevel)
{
    SWSS_LOG_ENTER();

    // We start from 1 since 0 is OTAI_API_UNSPECIFIED.

    for (uint32_t api = 1; api < otai_metadata_enum_otai_api_t.valuescount; ++api)
    {
        otai_status_t status = m_otai->logSet((otai_api_t)api, logLevel);

        if (status == OTAI_STATUS_SUCCESS)
        {
            SWSS_LOG_NOTICE("Setting OTAI loglevel %s on %s",
                otai_serialize_log_level(logLevel).c_str(),
                otai_serialize_api((otai_api_t)api).c_str());
        }
        else
        {
            SWSS_LOG_INFO("set loglevel failed: %s", otai_serialize_status(status).c_str());
        }
    }
}
