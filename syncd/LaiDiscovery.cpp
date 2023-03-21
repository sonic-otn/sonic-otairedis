#include "LaiDiscovery.h"

#include "swss/logger.h"

#include "meta/lai_serialize.h"

using namespace syncd;

/**
 * @def LAI_DISCOVERY_LIST_MAX_ELEMENTS
 *
 * Defines maximum elements that can be obtained from the OID list when
 * performing list attribute query (discovery) on the linecard.
 *
 * This value will be used to allocate memory on the stack for obtaining object
 * list, and should be big enough to obtain list for all ports on the linecard
 * and vlan members.
 */
#define LAI_DISCOVERY_LIST_MAX_ELEMENTS 1024

LaiDiscovery::LaiDiscovery(
    _In_ std::shared_ptr<lairedis::LaiInterface> lai) :
    m_lai(lai)
{
    SWSS_LOG_ENTER();

    // empty
}

LaiDiscovery::~LaiDiscovery()
{
    SWSS_LOG_ENTER();

    // empty
}

void LaiDiscovery::discover(
    _In_ lai_object_id_t rid,
    _Inout_ std::set<lai_object_id_t>& discovered)
{
    SWSS_LOG_ENTER();

    /*
     * NOTE: This method is only good after linecard init since we are making
     * assumptions that there are no ACL after initialization.
     *
     * NOTE: Input set could be a map of sets, this way we will also have
     * dependency on each oid.
     */

    if (rid == LAI_NULL_OBJECT_ID)
    {
        return;
    }

    if (discovered.find(rid) != discovered.end())
    {
        return;
    }

    lai_object_type_t ot = m_lai->objectTypeQuery(rid);

    if (ot == LAI_OBJECT_TYPE_NULL)
    {
        SWSS_LOG_THROW("objectTypeQuery: rid %s returned NULL object type",
            lai_serialize_object_id(rid).c_str());
    }

    SWSS_LOG_DEBUG("processing %s: %s",
        lai_serialize_object_id(rid).c_str(),
        lai_serialize_object_type(ot).c_str());

    discovered.insert(rid);

    const lai_object_type_info_t* info = lai_metadata_get_object_type_info(ot);

    /*
     * We will query only oid object types
     * then we don't need meta key, but we need to add to metadata
     * pointers to only generic functions.
     */

    lai_object_meta_key_t mk = { .objecttype = ot, .objectkey = {.key = {.object_id = rid } } };

    for (int idx = 0; info->attrmetadata[idx] != NULL; ++idx)
    {
        const lai_attr_metadata_t* md = info->attrmetadata[idx];

        /*
         * Note that we don't care about ACL object id's since
         * we assume that there are no ACLs on linecard after init.
         */

        lai_attribute_t attr;
        memset(&attr, 0, sizeof(attr));
        attr.id = md->attrid;

        if (md->attrvaluetype == LAI_ATTR_VALUE_TYPE_OBJECT_ID)
        {
            if (md->defaultvaluetype == LAI_DEFAULT_VALUE_TYPE_CONST)
            {
                /*
                 * This means that default value for this object is
                 * LAI_NULL_OBJECT_ID, since this is discovery after
                 * create, we don't need to query this attribute.
                 */

                 //continue;
            }

            SWSS_LOG_DEBUG("getting %s for %s", md->attridname,
                lai_serialize_object_id(rid).c_str());

            lai_status_t status = m_lai->get(mk.objecttype, mk.objectkey.key.object_id, 1, &attr);

            if (status != LAI_STATUS_SUCCESS)
            {
                /*
                 * We failed to get value, maybe it's not supported ?
                 */

                SWSS_LOG_INFO("%s: %s on %s",
                    md->attridname,
                    lai_serialize_status(status).c_str(),
                    lai_serialize_object_id(rid).c_str());

                continue;
            }

            m_defaultOidMap[rid][attr.id] = attr.value.oid;

            if (!md->allownullobjectid && attr.value.oid == LAI_NULL_OBJECT_ID)
            {
                // SWSS_LOG_WARN("got null on %s, but not allowed", md->attridname);
            }

            if (attr.value.oid != LAI_NULL_OBJECT_ID)
            {
                ot = m_lai->objectTypeQuery(attr.value.oid);

                if (ot == LAI_OBJECT_TYPE_NULL)
                {
                    SWSS_LOG_THROW("when query %s (on %s RID %s) got value %s objectTypeQuery returned NULL object type",
                        md->attridname,
                        lai_serialize_object_type(md->objecttype).c_str(),
                        lai_serialize_object_id(rid).c_str(),
                        lai_serialize_object_id(attr.value.oid).c_str());
                }
            }

            discover(attr.value.oid, discovered); // recursion
        }
        else if (md->attrvaluetype == LAI_ATTR_VALUE_TYPE_OBJECT_LIST)
        {
            if (md->defaultvaluetype == LAI_DEFAULT_VALUE_TYPE_EMPTY_LIST)
            {
                /*
                 * This means that default value for this object is
                 * empty list, since this is discovery after
                 * create, we don't need to query this attribute.
                 */

                 //continue;
            }

            SWSS_LOG_DEBUG("getting %s for %s", md->attridname,
                lai_serialize_object_id(rid).c_str());

            lai_object_id_t local[LAI_DISCOVERY_LIST_MAX_ELEMENTS];

            attr.value.objlist.count = LAI_DISCOVERY_LIST_MAX_ELEMENTS;
            attr.value.objlist.list = local;

            lai_status_t status = m_lai->get(mk.objecttype, mk.objectkey.key.object_id, 1, &attr);

            if (status != LAI_STATUS_SUCCESS)
            {
                /*
                 * We failed to get value, maybe it's not supported ?
                 */

                SWSS_LOG_INFO("%s: %s on %s",
                    md->attridname,
                    lai_serialize_status(status).c_str(),
                    lai_serialize_object_id(rid).c_str());

                continue;
            }

            SWSS_LOG_DEBUG("list count %s %u", md->attridname, attr.value.objlist.count);

            for (uint32_t i = 0; i < attr.value.objlist.count; ++i)
            {
                lai_object_id_t oid = attr.value.objlist.list[i];

                ot = m_lai->objectTypeQuery(oid);

                if (ot == LAI_OBJECT_TYPE_NULL)
                {
                    SWSS_LOG_THROW("when query %s (on %s RID %s) got value %s objectTypeQuery returned NULL object type",
                        md->attridname,
                        lai_serialize_object_type(md->objecttype).c_str(),
                        lai_serialize_object_id(rid).c_str(),
                        lai_serialize_object_id(oid).c_str());
                }

                discover(oid, discovered); // recursion
            }
        }
    }
}

std::set<lai_object_id_t> LaiDiscovery::discover(
    _In_ lai_object_id_t startRid)
{
    SWSS_LOG_ENTER();

    /*
     * Preform discovery on the linecard to obtain ASIC view of
     * objects that are created internally.
     */

    m_defaultOidMap.clear();

    std::set<lai_object_id_t> discovered_rids;

    {
        SWSS_LOG_TIMER("discover");

        //setApiLogLevel(LAI_LOG_LEVEL_CRITICAL);

        discover(startRid, discovered_rids);

        //setApiLogLevel(LAI_LOG_LEVEL_NOTICE);
    }

    SWSS_LOG_NOTICE("discovered objects count: %zu", discovered_rids.size());

    std::map<lai_object_type_t, int> map;

    for (lai_object_id_t rid : discovered_rids)
    {
        /*
         * We don't need to check for null since laiDiscovery already checked
         * that.
         */

        map[m_lai->objectTypeQuery(rid)]++;
    }

    for (const auto& p : map)
    {
        SWSS_LOG_NOTICE("%s: %d", lai_serialize_object_type(p.first).c_str(), p.second);
    }

    return discovered_rids;
}

const LaiDiscovery::DefaultOidMap& LaiDiscovery::getDefaultOidMap() const
{
    SWSS_LOG_ENTER();

    return m_defaultOidMap;
}

void LaiDiscovery::setApiLogLevel(
    _In_ lai_log_level_t logLevel)
{
    SWSS_LOG_ENTER();

    // We start from 1 since 0 is LAI_API_UNSPECIFIED.

    for (uint32_t api = 1; api < lai_metadata_enum_lai_api_t.valuescount; ++api)
    {
        lai_status_t status = m_lai->logSet((lai_api_t)api, logLevel);

        if (status == LAI_STATUS_SUCCESS)
        {
            SWSS_LOG_NOTICE("Setting LAI loglevel %s on %s",
                lai_serialize_log_level(logLevel).c_str(),
                lai_serialize_api((lai_api_t)api).c_str());
        }
        else
        {
            SWSS_LOG_INFO("set loglevel failed: %s", lai_serialize_status(status).c_str());
        }
    }
}
