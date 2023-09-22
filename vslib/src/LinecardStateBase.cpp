#include <thread>
#include <chrono>
#include <inttypes.h>

#include "LinecardStateBase.h"
#include "swss/logger.h"
#include "meta/otai_serialize.h"
#include "EventPayloadNotification.h"
#include "lib/inc/NotificationLinecardStateChange.h"
 
#include <net/if.h>

#include <algorithm>
#include <unistd.h>
#include <shell.h>
#include <string.h>

#define OTAI_VS_MAX_PORTS 1024

using namespace otaivs;
using namespace std;

int g_linecard_state_change = 0;
otai_oper_status_t g_linecard_state = OTAI_OPER_STATUS_DISABLED;

int g_alarm_change = 0;
bool g_alarm_occur = false;

int g_event_change = 0;
bool g_event_occur = false;

otai_object_id_t g_scanning_ocm_oid = OTAI_NULL_OBJECT_ID;
bool g_ocm_scan = false;

otai_object_id_t g_scanning_otdr_oid = OTAI_NULL_OBJECT_ID;
bool g_otdr_scan = false;

void LinecardStateBase::externEventThreadProc()
{
    SWSS_LOG_ENTER();

    while (m_externEventThreadRun)
    {
        if (g_linecard_state_change) {
            send_linecard_state_change_notification(0x100000000, g_linecard_state, true);
            g_linecard_state_change = 0;
        }

        if (g_alarm_change) {
            otai_alarm_type_t alarm_type = OTAI_ALARM_TYPE_RX_LOS;
            otai_alarm_info_t alarm_info;
            alarm_info.time_created = (uint64_t)chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch()).count();
            string text = "rx los";
            alarm_info.text.count = text.size();
            alarm_info.text.list = (int8_t *)text.c_str();

            alarm_info.resource_oid = 0x20000000e;
            alarm_info.severity = OTAI_ALARM_SEVERITY_MAJOR;

            if (g_alarm_occur)
            {
                alarm_info.status = OTAI_ALARM_STATUS_ACTIVE;
            }
            else
            {
                alarm_info.status = OTAI_ALARM_STATUS_INACTIVE;
            }
            send_linecard_alarm_notification(0x100000000, alarm_type, alarm_info);

            g_alarm_change = 0;
        }

        if (g_event_change) {
            if (g_event_occur)
            {
                otai_alarm_type_t alarm_type = OTAI_ALARM_TYPE_PORT_INIT;
                otai_alarm_info_t alarm_info;
                alarm_info.time_created = (uint64_t)chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch()).count();
                string text = "init port";
                alarm_info.text.count = text.size();
                alarm_info.text.list = (int8_t *)text.c_str();

                alarm_info.resource_oid = 0x1000000002;
                alarm_info.severity = OTAI_ALARM_SEVERITY_MINOR;
                alarm_info.status = OTAI_ALARM_STATUS_TRANSIENT;
                send_linecard_alarm_notification(0x100000000, alarm_type, alarm_info);
            }

            g_event_change = 0;
        }

        if (g_scanning_ocm_oid != OTAI_NULL_OBJECT_ID)
        {
            send_ocm_spectrum_power_notification(0x100000000, g_scanning_ocm_oid);
        }

        if (g_otdr_scan)
        {
            send_otdr_result_notification(0x100000000, g_scanning_otdr_oid); 
            g_otdr_scan = false;
        }

        usleep(1000);
    }
}

void LinecardStateBase::send_otdr_result_notification(
        _In_ otai_object_id_t linecard_id,
        _In_ otai_object_id_t otdr_id)
{
    SWSS_LOG_ENTER();

    otai_attribute_t attr;

    attr.id = OTAI_LINECARD_ATTR_LINECARD_OTDR_RESULT_NOTIFY;
    if (get(OTAI_OBJECT_TYPE_LINECARD, m_linecard_id, 1, &attr) != OTAI_STATUS_SUCCESS)
    {
        SWSS_LOG_ERROR("failed to get OTAI_LINECARD_ATTR_LINECARD_OTDR_RESULT_NOTIFY for linecard %s",
                       otai_serialize_object_id(m_linecard_id).c_str());

        return;
    }

    if (attr.value.ptr == NULL)
    {
        SWSS_LOG_INFO("OTAI_LINECARD_ATTR_LINECARD_OTDR_RESULT_NOTIFY callback is NULL");
        return;
    }

    usleep(100000);

    otai_otdr_result_t result;

    result.scanning_profile.scan_time = (uint64_t)chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch()).count();
    result.scanning_profile.distance_range = 80;
    result.scanning_profile.pulse_width = 20;
    result.scanning_profile.average_time = 300;
    result.scanning_profile.output_frequency = 125000;

    otai_otdr_event_t events[] = 
    {
        {
            .type = OTAI_OTDR_EVENT_TYPE_START,
            .length = 0.0,
            .loss = -40.0,
            .reflection = 1.0,
            .accumulate_loss = -40.0,
        },
        {
            .type = OTAI_OTDR_EVENT_TYPE_END,
            .length = 80.1,
            .loss = -38.1,
            .reflection = 1.0,
            .accumulate_loss = -38.0,
        }
    };

    result.events.span_distance = 80.1;
    result.events.span_loss = -38.0;
    result.events.events.count = 2;
    result.events.events.list = events;

    result.trace.update_time = result.scanning_profile.scan_time;

    uint8_t data[400000];
    uint8_t count = 0;
    for (uint32_t i = 0; i < sizeof(data)/sizeof(uint8_t); i++)
    {
        data[i] = count++;
    }

    result.trace.data.count = sizeof(data)/sizeof(uint8_t);
    result.trace.data.list = data;

    otai_linecard_notifications_t mn = {nullptr, nullptr, nullptr, nullptr};
    mn.on_linecard_otdr_result = (otai_linecard_otdr_result_notification_fn)attr.value.ptr;
    mn.on_linecard_otdr_result(linecard_id, otdr_id, result);

    otai_attribute_t otdr_attr;
    otdr_attr.id = OTAI_OTDR_ATTR_SCANNING_STATUS;
    otdr_attr.value.s32 = OTAI_SCANNING_STATUS_INACTIVE;

    for (auto &strOid : m_otdrOidList)
    {
        set(OTAI_OBJECT_TYPE_OTDR, strOid, &otdr_attr);
    }
}

void LinecardStateBase::updateObjectThreadProc()
{
    SWSS_LOG_ENTER();

    sleep(15);

    while (m_updateObjectThreadRun)
    {
        for (auto &i : m_objectHash)
        {
            for (auto &j : i.second)
            {
                for (auto &k : j.second)
                {
                    k.second->updateValue();

                }
            }
        }      
        sleep(1);
    }
}

LinecardStateBase::LinecardStateBase(
        _In_ otai_object_id_t linecard_id,
        _In_ std::shared_ptr<RealObjectIdManager> manager,
        _In_ std::shared_ptr<LinecardConfig> config):
    LinecardState(linecard_id, config),
    m_realObjectIdManager(manager)
{
    SWSS_LOG_ENTER();

    m_externEventThreadRun = true;
    m_externEventThread = std::make_shared<std::thread>(&LinecardStateBase::externEventThreadProc, this);
    m_updateObjectThreadRun = false;
    m_updateObjectThread = std::make_shared<std::thread>(&LinecardStateBase::updateObjectThreadProc, this);
}

LinecardStateBase::~LinecardStateBase()
{
    SWSS_LOG_ENTER();

    m_externEventThreadRun = false;
    m_externEventThread->join();
    // empty
}

otai_status_t LinecardStateBase::create(
        _In_ otai_object_type_t object_type,
        _Out_ otai_object_id_t *object_id,
        _In_ otai_object_id_t linecard_id,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    if (object_type == OTAI_OBJECT_TYPE_LINECARD)
    {
        SWSS_LOG_THROW("this method can't be used to create linecard");
    }

    *object_id = m_realObjectIdManager->allocateNewObjectId(object_type, linecard_id, attr_count, attr_list);

    auto sid = otai_serialize_object_id(*object_id);

    return create_internal(object_type, sid, linecard_id, attr_count, attr_list);
}

otai_status_t LinecardStateBase::create(
        _In_ otai_object_type_t object_type,
        _In_ const std::string &serializedObjectId,
        _In_ otai_object_id_t linecard_id,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return create_internal(object_type, serializedObjectId, linecard_id, attr_count, attr_list);
}

void LinecardStateBase::setObjectHash(
        _In_ otai_object_type_t object_type,
        _In_ const std::string &serializedObjectId,
        _In_ const otai_attribute_t *attr)
{
    auto &objectHash = m_objectHash.at(object_type);
    auto a = std::make_shared<OtaiAttrWrap>(object_type, attr);
    objectHash[serializedObjectId][a->getAttrMetadata()->attridname] = a;
}

otai_status_t LinecardStateBase::create_internal(
        _In_ otai_object_type_t object_type,
        _In_ const std::string &serializedObjectId,
        _In_ otai_object_id_t linecard_id,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();
    auto &objectHash = m_objectHash.at(object_type);

    if (m_linecardConfig->m_resourceLimiter)
    {
        size_t limit = m_linecardConfig->m_resourceLimiter->getObjectTypeLimit(object_type);

        if (objectHash.size() >= limit)
        {
            SWSS_LOG_ERROR("too many %s, created %zu is resource limit",
                    otai_serialize_object_type(object_type).c_str(),
                    limit);

            return OTAI_STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    
    #if 0
    auto it = objectHash.find(serializedObjectId);
    if (object_type != OTAI_OBJECT_TYPE_LINECARD)
    {
        /*
         * Linecard is special, and object is already created by init.
         *
         * XXX revisit this.
         */

        if (it != objectHash.end())
        {
            SWSS_LOG_ERROR("create failed, object already exists, object type: %s: id: %s",
                    otai_serialize_object_type(object_type).c_str(),
                    serializedObjectId.c_str());

            return OTAI_STATUS_ITEM_ALREADY_EXISTS;
        }
    }
    #endif

    if (objectHash.find(serializedObjectId) == objectHash.end())
    {
        /*
         * Number of attributes may be zero, so see if actual entry was created
         * with empty hash.
         */

        objectHash[serializedObjectId] = {};

        otai_attr_id_t id = 0;
        otai_attribute_t attr;
        const otai_attr_metadata_t *meta = \
           otai_metadata_get_attr_metadata(object_type, id);

        while (meta != NULL)
        {
            attr.id = id;
            switch (meta->attrvaluetype)
            {
                case OTAI_ATTR_VALUE_TYPE_BOOL:
                    attr.value.booldata = true;
                    setObjectHash(object_type, serializedObjectId, &attr);
                    break;
                case OTAI_ATTR_VALUE_TYPE_UINT8:
                    attr.value.u8 = 0;
                    setObjectHash(object_type, serializedObjectId, &attr);
                    break;
                case OTAI_ATTR_VALUE_TYPE_INT8:
                    attr.value.s8 = 0;
                    setObjectHash(object_type, serializedObjectId, &attr);
                    break;
                case OTAI_ATTR_VALUE_TYPE_UINT16:
                    attr.value.u16 = 0;
                    setObjectHash(object_type, serializedObjectId, &attr);
                    break;
                case OTAI_ATTR_VALUE_TYPE_INT16:
                    attr.value.s16 = 0;
                    setObjectHash(object_type, serializedObjectId, &attr);
                    break;
                case OTAI_ATTR_VALUE_TYPE_UINT32:
                    attr.value.u32 = 0;
                    setObjectHash(object_type, serializedObjectId, &attr);
                    break;
                case OTAI_ATTR_VALUE_TYPE_INT32:
                    attr.value.s32 = 0;
                    setObjectHash(object_type, serializedObjectId, &attr);
                    break;
                case OTAI_ATTR_VALUE_TYPE_UINT64:
                    attr.value.u64 = 0;
                    setObjectHash(object_type, serializedObjectId, &attr);
                    break;
                case OTAI_ATTR_VALUE_TYPE_INT64:
                    attr.value.s64 = 0;
                    setObjectHash(object_type, serializedObjectId, &attr);
                    break;
                case OTAI_ATTR_VALUE_TYPE_DOUBLE:
                    attr.value.d64 = 0.0;
                    setObjectHash(object_type, serializedObjectId, &attr);
                    break;
                case OTAI_ATTR_VALUE_TYPE_CHARDATA:
                    strncpy(attr.value.chardata, "test data", sizeof(attr.value.chardata));
                    setObjectHash(object_type, serializedObjectId, &attr);
                    break;
                default:
                    break;
            }
            id++;    
            meta = otai_metadata_get_attr_metadata(object_type, id);
        }

        if (object_type == OTAI_OBJECT_TYPE_OTDR)
        {
            attr.id = OTAI_OTDR_ATTR_SCANNING_STATUS;
            attr.value.s32 = OTAI_SCANNING_STATUS_INACTIVE;
            setObjectHash(object_type, serializedObjectId, &attr);

            m_otdrOidList.insert(serializedObjectId);
        }
    }

    for (uint32_t i = 0; i < attr_count; ++i)
    {
        auto a = std::make_shared<OtaiAttrWrap>(object_type, &attr_list[i]);

        objectHash[serializedObjectId][a->getAttrMetadata()->attridname] = a;
    }

    return OTAI_STATUS_SUCCESS;
}

otai_status_t LinecardStateBase::remove(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    auto sid = otai_serialize_object_id(objectId);

    return remove(object_type, sid);
}

otai_status_t LinecardStateBase::remove(
        _In_ otai_object_type_t object_type,
        _In_ const std::string &serializedObjectId)
{
    SWSS_LOG_ENTER();

    return remove_internal(object_type, serializedObjectId);
}

otai_status_t LinecardStateBase::remove_internal(
        _In_ otai_object_type_t object_type,
        _In_ const std::string &serializedObjectId)
{
    SWSS_LOG_ENTER();

    auto &objectHash = m_objectHash.at(object_type);

    auto it = objectHash.find(serializedObjectId);

    if (it == objectHash.end())
    {
        SWSS_LOG_ERROR("not found %s:%s",
                otai_serialize_object_type(object_type).c_str(),
                serializedObjectId.c_str());

        return OTAI_STATUS_ITEM_NOT_FOUND;
    }

    objectHash.erase(it);

    return OTAI_STATUS_SUCCESS;
}

otai_status_t LinecardStateBase::set(
        _In_ otai_object_type_t objectType,
        _In_ const std::string &serializedObjectId,
        _In_ const otai_attribute_t* attr)
{
    SWSS_LOG_ENTER();

    return set_internal(objectType, serializedObjectId, attr);
}

otai_status_t LinecardStateBase::set_internal(
        _In_ otai_object_type_t objectType,
        _In_ const std::string &serializedObjectId,
        _In_ const otai_attribute_t* attr)
{
    SWSS_LOG_ENTER();

    auto it = m_objectHash.at(objectType).find(serializedObjectId);

    if (it == m_objectHash.at(objectType).end())
    {
        SWSS_LOG_ERROR("not found %s:%s",
                otai_serialize_object_type(objectType).c_str(),
                serializedObjectId.c_str());

        return OTAI_STATUS_ITEM_NOT_FOUND;
    }

    auto &attrHash = it->second;

    auto a = std::make_shared<OtaiAttrWrap>(objectType, attr);

    // set have only one attribute
    attrHash[a->getAttrMetadata()->attridname] = a;

    auto meta = otai_metadata_get_attr_metadata(objectType, attr->id);

    SWSS_LOG_INFO("set %s:%s %s", otai_serialize_object_type(objectType).c_str(),
                  serializedObjectId.c_str(),
                  meta->attridname);

    if (objectType == OTAI_OBJECT_TYPE_OTDR &&
        attr->id == OTAI_OTDR_ATTR_SCAN &&
        attr->value.booldata == true)
    {
        if (g_otdr_scan)
        {
            return OTAI_STATUS_FAILURE;
        }

        otai_attribute_t otdr_attr;
        otdr_attr.id = OTAI_OTDR_ATTR_SCANNING_STATUS;
        otdr_attr.value.s32 = OTAI_SCANNING_STATUS_ACTIVE;

        for (auto &strOid : m_otdrOidList)
        {
            set(OTAI_OBJECT_TYPE_OTDR, strOid, &otdr_attr);
        }

        otai_deserialize_object_id(serializedObjectId, g_scanning_otdr_oid);

        g_otdr_scan = true;
    }

    return OTAI_STATUS_SUCCESS;
}

otai_status_t LinecardStateBase::set(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ const otai_attribute_t* attr)
{
    SWSS_LOG_ENTER();

    auto sid = otai_serialize_object_id(objectId);

    return set(objectType, sid, attr);
}

otai_status_t LinecardStateBase::get(
        _In_ otai_object_type_t object_type,
        _In_ otai_object_id_t object_id,
        _In_ uint32_t attr_count,
        _Out_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    auto sid = otai_serialize_object_id(object_id);
    
    return get(object_type, sid, attr_count, attr_list);
}

otai_status_t LinecardStateBase::get(
        _In_ otai_object_type_t objectType,
        _In_ const std::string &serializedObjectId,
        _In_ uint32_t attr_count,
        _Out_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();
    const auto &objectHash = m_objectHash.at(objectType);

    auto it = objectHash.find(serializedObjectId);

    if (it == objectHash.end())
    {
        SWSS_LOG_ERROR("not found %s:%s",
                otai_serialize_object_type(objectType).c_str(),
                serializedObjectId.c_str());

        return OTAI_STATUS_ITEM_NOT_FOUND;
    }
    /*
     * We need reference here since we can potentially update attr hash for RO
     * object.
     */

    auto& attrHash = it->second;

    /*
     * Some of the list query maybe for length, so we can't do
     * normal serialize, maybe with count only.
     */

    otai_status_t final_status = OTAI_STATUS_SUCCESS;

    for (uint32_t idx = 0; idx < attr_count; ++idx)
    {
        otai_attr_id_t id = attr_list[idx].id;

        auto meta = otai_metadata_get_attr_metadata(objectType, id);

        if (meta == NULL)
        {
            SWSS_LOG_ERROR("failed to find attribute %d for %s:%s", id,
                    otai_serialize_object_type(objectType).c_str(),
                    serializedObjectId.c_str());

            return OTAI_STATUS_FAILURE;
        }
        otai_status_t status;

        if (OTAI_HAS_FLAG_READ_ONLY(meta->flags))
        {
            /*
             * Read only attributes may require recalculation.
             * Metadata makes sure that non object id's can't have
             * read only attributes. So here is definitely OID.
             */

            otai_object_id_t oid;
            otai_deserialize_object_id(serializedObjectId, oid);

            status = refresh_read_only(meta, oid);

            if (status != OTAI_STATUS_SUCCESS)
            {
                SWSS_LOG_INFO("%s read only not implemented on %s",
                        meta->attridname,
                        serializedObjectId.c_str());

                return status;
            }
        }

        auto ait = attrHash.find(meta->attridname);

        if (ait == attrHash.end())
        {
            SWSS_LOG_INFO("%s not implemented on %s",
                    meta->attridname,
                    serializedObjectId.c_str());
             SWSS_LOG_NOTICE("LinecardStateBase attridname is %s",meta->attridname);
            return OTAI_STATUS_NOT_IMPLEMENTED;
        }

        auto attr = ait->second->getAttr();

        status = transfer_attributes(objectType, 1, attr, &attr_list[idx], false);

        if (status == OTAI_STATUS_BUFFER_OVERFLOW)
        {
            /*
             * This is considered partial success, since we get correct list
             * length.  Note that other items ARE processes on the list.
             */

            SWSS_LOG_NOTICE("BUFFER_OVERFLOW %s: %s",
                    serializedObjectId.c_str(),
                    meta->attridname);

            /*
             * We still continue processing other attributes for get as long as
             * we only will be getting buffer overflow error.
             */

            final_status = status;
            continue;
        }

        if (status != OTAI_STATUS_SUCCESS)
        {
            // all other errors

            SWSS_LOG_ERROR("get failed %s: %s: %s",
                    serializedObjectId.c_str(),
                    meta->attridname,
                    otai_serialize_status(status).c_str());
            return status;
        }
    }
    return final_status;
}

otai_status_t LinecardStateBase::set_linecard_default_attributes()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_INFO("create linecard default attributes");

    otai_attribute_t attr;

    attr.id = OTAI_LINECARD_ATTR_LINECARD_TYPE;
    strcpy(attr.value.chardata, "P230C");

    return set(OTAI_OBJECT_TYPE_LINECARD, m_linecard_id, &attr);
}

otai_status_t LinecardStateBase::initialize_default_objects(
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_SUCCESS;
}

// XXX extra work may be needed on GET api if N on list will be > then actual

/*
 * We can use local variable here for initialization (init should be in class
 * constructor anyway, we can move it there later) because each linecard init is
 * done under global lock.
 */

/*
 * NOTE For recalculation we can add flag on create/remove specific object type
 * so we can deduce whether actually need to perform recalculation, as
 * optimization.
 */

otai_status_t LinecardStateBase::refresh_read_only(
        _In_ const otai_attr_metadata_t *meta,
        _In_ otai_object_id_t object_id)
{
    SWSS_LOG_ENTER();

    if (meta->objecttype == OTAI_OBJECT_TYPE_LINECARD)
    {
        return OTAI_STATUS_SUCCESS;
    }

    if (meta->objecttype == OTAI_OBJECT_TYPE_TRANSCEIVER ||
        meta->objecttype == OTAI_OBJECT_TYPE_OTN ||
        meta->objecttype == OTAI_OBJECT_TYPE_LOGICALCHANNEL ||
        meta->objecttype == OTAI_OBJECT_TYPE_ETHERNET ||
        meta->objecttype == OTAI_OBJECT_TYPE_PHYSICALCHANNEL ||
        meta->objecttype == OTAI_OBJECT_TYPE_OCH ||
        meta->objecttype == OTAI_OBJECT_TYPE_PORT ||
        meta->objecttype == OTAI_OBJECT_TYPE_ASSIGNMENT ||
        meta->objecttype == OTAI_OBJECT_TYPE_INTERFACE ||
        meta->objecttype == OTAI_OBJECT_TYPE_OA ||
        meta->objecttype == OTAI_OBJECT_TYPE_OSC ||
        meta->objecttype == OTAI_OBJECT_TYPE_APS ||
        meta->objecttype == OTAI_OBJECT_TYPE_APSPORT ||
        meta->objecttype == OTAI_OBJECT_TYPE_ATTENUATOR ||
        meta->objecttype == OTAI_OBJECT_TYPE_OCM ||
        meta->objecttype == OTAI_OBJECT_TYPE_OTDR ||
        meta->objecttype == OTAI_OBJECT_TYPE_LLDP)
    {
        return OTAI_STATUS_SUCCESS;
    }

    auto mmeta = m_meta.lock();

    if (mmeta)
    {
        if (mmeta->meta_unittests_enabled())
        {
            SWSS_LOG_NOTICE("unittests enabled, SET could be performed on %s, not recalculating", meta->attridname);

            return OTAI_STATUS_SUCCESS;
        }
    }
    else
    {
        SWSS_LOG_WARN("meta pointer expired");
    }

    SWSS_LOG_WARN("need to recalculate RO: %s", meta->attridname);

    return OTAI_STATUS_NOT_IMPLEMENTED;
}

bool LinecardStateBase::check_object_default_state(
        _In_ otai_object_id_t object_id)
{
    SWSS_LOG_ENTER();

    otai_object_type_t object_type = objectTypeQuery(object_id);

    if (object_type == OTAI_OBJECT_TYPE_NULL)
    {
        SWSS_LOG_ERROR("failed to get object type for oid: %s",
                otai_serialize_object_id(object_id).c_str());

        return false;
    }

    auto* oti = otai_metadata_get_object_type_info(object_type);

    if (oti == nullptr)
    {
        SWSS_LOG_THROW("failed to get object type info for object type: %s",
                otai_serialize_object_type(object_type).c_str());
    }

    // iterate over all attributes

    for (size_t i = 0; i < oti->attrmetadatalength; i++)
    {
        auto* meta = oti->attrmetadata[i];

        // skip readonly, mandatory on create and non oid attributes

        if (meta->isreadonly)
            continue;

        if (!meta->isoidattribute)
            continue;

        // here we have only oid/object list attrs and we expect each of this
        // attribute will be in default state which for oid is usually NULL,
        // and for object list is empty

        otai_attribute_t attr;

        attr.id = meta->attrid;

        otai_status_t status;

        std::vector<otai_object_id_t> objlist;

        if (meta->attrvaluetype == OTAI_ATTR_VALUE_TYPE_OBJECT_ID)
        {
            // ok
        }
        else if (meta->attrvaluetype == OTAI_ATTR_VALUE_TYPE_OBJECT_LIST)
        {
            objlist.resize(MAX_OBJLIST_LEN);

            attr.value.objlist.count = MAX_OBJLIST_LEN;
            attr.value.objlist.list = objlist.data();
        }
        else
        {
            // unable to check whether object is in default state, need fix

            SWSS_LOG_ERROR("unsupported oid attribute: %s, FIX ME!", meta->attridname);
            return false;
        }

        status = get(object_type, object_id, 1, &attr);

        switch (status)
        {
            case OTAI_STATUS_NOT_IMPLEMENTED:
            case OTAI_STATUS_NOT_SUPPORTED:
                continue;

            case OTAI_STATUS_SUCCESS:
                break;

            default:

                SWSS_LOG_ERROR("unexpected status %s on %s obj %s",
                        otai_serialize_status(status).c_str(),
                        meta->attridname,
                        otai_serialize_object_id(object_id).c_str());
                return false;

        }


        if (meta->attrvaluetype == OTAI_ATTR_VALUE_TYPE_OBJECT_ID)
        {
            if (attr.value.oid != OTAI_NULL_OBJECT_ID)
            {
                SWSS_LOG_ERROR("expected null object id on %s on %s, but got: %s",
                        meta->attridname,
                        otai_serialize_object_id(object_id).c_str(),
                        otai_serialize_object_id(attr.value.oid).c_str());

                return false;
            }

        }
        else if (meta->attrvaluetype == OTAI_ATTR_VALUE_TYPE_OBJECT_LIST)
        {
            if (objlist.size())
            {
                SWSS_LOG_ERROR("expected empty list on %s on %s, contents:",
                        meta->attridname,
                        otai_serialize_object_id(object_id).c_str());

                for (auto oid: objlist)
                {
                    SWSS_LOG_ERROR(" - oid: %s", otai_serialize_object_id(oid).c_str());
                }

                return false;
            }
        }
        else
        {
            // unable to check whether object is in default state, need fix

            SWSS_LOG_ERROR("unsupported oid attribute: %s, FIX ME!", meta->attridname);
            return false;
        }
    }

    // XXX later there can be issue when we for example add extra queues to
    // the port those new queues should be removed by user first before
    // removing port, and currently we don't have a way to differentiate those

    // object is in default state
    return true;
}

bool LinecardStateBase::check_object_list_default_state(
        _Out_ const std::vector<otai_object_id_t>& objlist)
{
    SWSS_LOG_ENTER();

    return std::all_of(objlist.begin(), objlist.end(),
            [&](otai_object_id_t oid) { return check_object_default_state(oid); });
}

otai_object_type_t LinecardStateBase::objectTypeQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return RealObjectIdManager::objectTypeQuery(objectId);
}

otai_object_id_t LinecardStateBase::linecardIdQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return RealObjectIdManager::linecardIdQuery(objectId);
}

void LinecardStateBase::findObjects(
        _In_ otai_object_type_t object_type,
        _In_ const otai_attribute_t &expect,
        _Out_ std::vector<otai_object_id_t> &objects)
{
    SWSS_LOG_ENTER();

    objects.clear();

    OtaiAttrWrap expect_wrap(object_type, &expect);

    for (auto &obj : m_objectHash.at(object_type))
    {
        auto attr_itr = obj.second.find(expect_wrap.getAttrMetadata()->attridname);

        if (attr_itr != obj.second.end()
                && attr_itr->second->getAttrStrValue() == expect_wrap.getAttrStrValue())
        {
            otai_object_id_t object_id;
            otai_deserialize_object_id(obj.first, object_id);
            objects.push_back(object_id);
        }
    }
}

bool LinecardStateBase::dumpObject(
        _In_ const otai_object_id_t object_id,
        _Out_ std::vector<otai_attribute_t> &attrs)
{
    SWSS_LOG_ENTER();

    attrs.clear();

    auto &objs = m_objectHash.at(objectTypeQuery(object_id));
    auto obj = objs.find(otai_serialize_object_id(object_id));

    if (obj == objs.end())
    {
        return false;
    }

    for (auto &attr : obj->second)
    {
        attrs.push_back(*attr.second->getAttr());
    }

    return true;
}

void LinecardStateBase::send_aps_switch_info_notification(
    _In_ otai_object_id_t linecard_id
)
{
    SWSS_LOG_ENTER();

    otai_attribute_t attr;
    attr.id = OTAI_APS_ATTR_SWITCH_INFO_NOTIFY;

    if (get(OTAI_OBJECT_TYPE_APS, linecard_id, 1, &attr) != OTAI_STATUS_SUCCESS) {
        SWSS_LOG_ERROR("failed to get OTAI_APS_ATTR_SWITCH_INFO_NOTIFY");
        return;
    }
    if (attr.value.ptr == NULL) {
        SWSS_LOG_INFO("OTAI_APS_ATTR_SWITCH_INFO_NOTIFY callback is NULL");
        return;
    }
    otai_olp_switch_t switch_info;
    otai_aps_report_switch_info_fn fn = (otai_aps_report_switch_info_fn)(attr.value.ptr);
    fn(0, switch_info);
    SWSS_LOG_NOTICE("Finish report switch info");
}

void LinecardStateBase::send_linecard_state_change_notification(
        _In_ otai_object_id_t linecard_id,
        _In_ otai_oper_status_t status,
        _In_ bool force)
{
    SWSS_LOG_ENTER();

    auto objectType = objectTypeQuery(linecard_id);

    if (objectType != OTAI_OBJECT_TYPE_LINECARD) {
        SWSS_LOG_ERROR("object type %s not supported on linecard_id %s",
                otai_serialize_object_type(objectType).c_str(),
                otai_serialize_object_id(linecard_id).c_str());
        return;  
    }

    otai_attribute_t attr;

    attr.id = OTAI_LINECARD_ATTR_OPER_STATUS;

    if (get(objectType, linecard_id, 1, &attr) != OTAI_STATUS_SUCCESS) {
        SWSS_LOG_ERROR("failed to get linecard attribute OTAI_LINECARD_ATTR_OPER_STATUS");
    } else {
        if (force) {
            SWSS_LOG_NOTICE("explicitly send OTAI_LINECARD_ATTR_LINECARD_STATE_CHANGE_NOTIFY for linecard %s",
                        otai_serialize_object_id(linecard_id).c_str());
        } else if ((otai_oper_status_t)attr.value.s32 == status) {
            SWSS_LOG_INFO("linecard oper status didn't changed, will not send notification");
            return;
        }
    }

    attr.id = OTAI_LINECARD_ATTR_LINECARD_STATE_CHANGE_NOTIFY;

    if (get(OTAI_OBJECT_TYPE_LINECARD, m_linecard_id, 1, &attr) != OTAI_STATUS_SUCCESS) {
        SWSS_LOG_ERROR("failed to get OTAI_LINECARD_ATTR_LINECARD_STATE_CHANGE_NOTIFY for linecard %s",
                otai_serialize_object_id(m_linecard_id).c_str());
        return;
    }

    if (attr.value.ptr == NULL) {
        SWSS_LOG_INFO("OTAI_LINECARD_ATTR_LINECARD_STATE_CHANGE_NOTIFY callback is NULL");
        return;
    }

    otai_linecard_notifications_t mn = {nullptr, nullptr, nullptr, nullptr};
    mn.on_linecard_state_change = (otai_linecard_state_change_notification_fn)attr.value.ptr;
    mn.on_linecard_state_change(linecard_id,status);  
}

void LinecardStateBase::send_linecard_alarm_notification(
        _In_ otai_object_id_t linecard_id,
        _In_ otai_alarm_type_t alarm_type,
        _In_ otai_alarm_info_t alarm_info)
{
    SWSS_LOG_ENTER();
    auto objectType = objectTypeQuery(linecard_id);
    if (objectType != OTAI_OBJECT_TYPE_LINECARD) {
        SWSS_LOG_ERROR("object type %s not supported on linecard_id %s",
                otai_serialize_object_type(objectType).c_str(),
                otai_serialize_object_id(linecard_id).c_str());
        return;  
    }

    otai_attribute_t attr;
    attr.id = OTAI_LINECARD_ATTR_LINECARD_ALARM_NOTIFY;
    if (get(OTAI_OBJECT_TYPE_LINECARD, m_linecard_id, 1, &attr) != OTAI_STATUS_SUCCESS) {
        SWSS_LOG_ERROR("failed to get OTAI_LINECARD_ATTR_LINECARD_STATE_CHANGE_NOTIFY for linecard %s",
            otai_serialize_object_id(m_linecard_id).c_str());
        return;
    }
    
    if (attr.value.ptr == NULL) {
    	SWSS_LOG_INFO("OTAI_LINECARD_ATTR_LINECARD_STATE_CHANGE_NOTIFY callback is NULL");
    	return;
    }
    
    otai_linecard_notifications_t mn = {nullptr, nullptr, nullptr, nullptr};
    mn.on_linecard_alarm = (otai_linecard_alarm_notification_fn)attr.value.ptr;
    mn.on_linecard_alarm(linecard_id,alarm_type,alarm_info);  
}

void LinecardStateBase::send_ocm_spectrum_power_notification(
        _In_ otai_object_id_t linecard_id,
        _In_ otai_object_id_t ocm_id)
{
    otai_attribute_t attr;

    attr.id = OTAI_LINECARD_ATTR_LINECARD_OCM_SPECTRUM_POWER_NOTIFY;
    if (get(OTAI_OBJECT_TYPE_LINECARD, m_linecard_id, 1, &attr) != OTAI_STATUS_SUCCESS) {
        SWSS_LOG_ERROR("failed to get OTAI_LINECARD_ATTR_LINECARD_OCM_SPECTRUM_POWER_NOTIFY for linecard %s",
            otai_serialize_object_id(m_linecard_id).c_str());
        return;
    }
    
    if (attr.value.ptr == NULL) {
        SWSS_LOG_INFO("OTAI_LINECARD_ATTR_LINECARD_OCM_SPECTRUM_POWER_NOTIFY callback is NULL");
    	return;
    }

    usleep(1000000);

    otai_spectrum_power_t list[96];
    otai_uint64_t freq = 194400000;
    otai_uint64_t freq_step = 12500;
    for (int i = 0; i < 96; i++)
    {
        list[i].lower_frequency = freq;
        list[i].upper_frequency = freq + freq_step;
        list[i].power = -10.0;

        freq += freq_step;
    }

    otai_spectrum_power_list_t ocm_result;
    ocm_result.count = 96;
    ocm_result.list = list; 

    otai_linecard_notifications_t mn = {nullptr, nullptr, nullptr, nullptr};
    mn.on_linecard_ocm_spectrum_power = (otai_linecard_ocm_spectrum_power_notification_fn)attr.value.ptr;
    mn.on_linecard_ocm_spectrum_power(linecard_id, ocm_id, ocm_result);
}


