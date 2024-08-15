#include <thread>
#include <chrono>
#include <inttypes.h>

#include "SoftReiniter.h"
#include "FlexCounterReiniter.h"
#include "VidManager.h"
#include "RedisClient.h"
#include "swss/logger.h"
#include "meta/otai_serialize.h"

using namespace syncd;
using namespace otaimeta;
using namespace std;

SoftReiniter::SoftReiniter(
    _In_ shared_ptr<RedisClient> client,
    _In_ std::shared_ptr<VirtualOidTranslator> translator,
    _In_ std::shared_ptr<otairedis::OtaiInterface> otai,
    _In_ std::shared_ptr<FlexCounterManager> manager):
    m_vendorOtai(otai),
    m_translator(translator),
    m_client(client),
    m_manager(manager)
{
    SWSS_LOG_ENTER();
}

SoftReiniter::~SoftReiniter()
{
    SWSS_LOG_ENTER();
}

void SoftReiniter::readAsicState()
{
    SWSS_LOG_ENTER();

    SWSS_LOG_TIMER("read asic state");

    m_vidToRidMap = m_client->getVidToRidMap();
    m_ridToVidMap = m_client->getRidToVidMap();

    for (auto& v2r: m_vidToRidMap) {
        auto linecardId = VidManager::linecardIdQuery(v2r.first);
        m_linecardVidToRid[linecardId][v2r.first] = v2r.second;
    }

    for (auto& r2v: m_ridToVidMap) {
        auto linecardId = VidManager::linecardIdQuery(r2v.second);
        m_linecardRidToVid[linecardId][r2v.first] = r2v.second;
    }

    auto asicStateKeys = m_client->getAsicStateKeys();
    for (const auto &key: asicStateKeys) {
        auto mk = key.substr(key.find_first_of(":") + 1); // skip asic key

        otai_object_meta_key_t metaKey;
        otai_deserialize_object_meta_key(mk, metaKey);

        // if object is non object id then first item will be linecard id
        auto linecardId = VidManager::linecardIdQuery(metaKey.objectkey.key.object_id);

        m_linecardMap[linecardId].push_back(key);
    }   
    SWSS_LOG_NOTICE("loaded %zu linecards", m_linecardMap.size());

    for (auto& kvp: m_linecardMap) {
        SWSS_LOG_NOTICE("linecard VID: %s keys %d", otai_serialize_object_id(kvp.first).c_str(), kvp.second.size());
    }
}

void SoftReiniter::prepareAsicState(vector<string> &asicKeys)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_TIMER("read asic state asicKeys %d", (int)asicKeys.size());

    for (auto &key : asicKeys) {
        otai_object_type_t objectType = getObjectTypeFromAsicKey(key);
        const string& strObjectId = getObjectIdFromAsicKey(key);
        string strObjectType = otai_serialize_object_type(objectType);

        SWSS_LOG_NOTICE("objectType = %s, objectId=%s", strObjectType.c_str(), strObjectId.c_str());

        auto info = otai_metadata_get_object_type_info(objectType);
        switch (objectType) {
        case OTAI_OBJECT_TYPE_LINECARD:
            m_linecards[strObjectId] = key;
            m_oids[strObjectId] = key;
            break;
        default:
            if (info->isnonobjectid) {
                SWSS_LOG_THROW("passing non object id %s as generic object", info->objecttypename);
            }
            m_oids[strObjectId] = key;
            break;
        }
        m_attributesLists[key] = redisGetAttributesFromAsicKey(key);
    }
}

void SoftReiniter::stopPreConfigLinecards()
{
    SWSS_LOG_ENTER();

    if (m_linecards.size() > 1) {
        SWSS_LOG_THROW("multiple linecards %zu in single hard reinit are not allowed", m_linecards.size());
    }

    for (const auto& s : m_linecards) {
        std::string strLinecardVid = s.first;

        otai_deserialize_object_id(strLinecardVid, m_linecard_vid); 
        if (m_linecard_vid == OTAI_NULL_OBJECT_ID) {
            SWSS_LOG_THROW("linecard id can't be NULL");
        }
        auto oit = m_oids.find(strLinecardVid);
        if (oit == m_oids.end()) {
            SWSS_LOG_THROW("failed to find VID %s in OIDs map", strLinecardVid.c_str());
        }
        m_linecard_rid = m_vidToRidMap[m_linecard_vid];

        otai_attribute_t attr;
        attr.id = OTAI_LINECARD_ATTR_STOP_PRE_CONFIGURATION;
        attr.value.booldata = true;

        SWSS_LOG_NOTICE("Stop pre-config linecard");

        otai_status_t status = m_vendorOtai->set(OTAI_OBJECT_TYPE_LINECARD, m_linecard_rid, &attr);
        if (status != OTAI_STATUS_SUCCESS) {
            SWSS_LOG_THROW("failed to stop pre-config linecard");
        }
    }
}

void SoftReiniter::setBoardMode(std::string mode)
{
    SWSS_LOG_ENTER();

    int wait_count = 0;
    otai_attribute_t attr;
    otai_status_t status;

    attr.id = OTAI_LINECARD_ATTR_BOARD_MODE;
    memset(attr.value.chardata, 0, sizeof(attr.value.chardata));
    status = m_vendorOtai->get(OTAI_OBJECT_TYPE_LINECARD, m_linecard_rid, 1, &attr);
    if (status == OTAI_STATUS_SUCCESS && mode == attr.value.chardata)
    {   
        SWSS_LOG_DEBUG("Linecard and maincard have a same board-mode, %s", mode.c_str());
        return;
    }   

    SWSS_LOG_NOTICE("Begin to set board-mode %s", mode.c_str());

    memset(attr.value.chardata, 0, sizeof(attr.value.chardata));
    strncpy(attr.value.chardata, mode.c_str(), sizeof(attr.value.chardata) - 1);
    status = m_vendorOtai->set(OTAI_OBJECT_TYPE_LINECARD, m_linecard_rid, &attr);
    if (status != OTAI_STATUS_SUCCESS)
    {   
        SWSS_LOG_ERROR("Failed to set board-mode status=%d, mode=%s",
                       status, mode.c_str());
        return;
    }
    do
    {
        wait_count++;
        this_thread::sleep_for(chrono::milliseconds(1000));
        memset(attr.value.chardata, 0, sizeof(attr.value.chardata));
        status = m_vendorOtai->get(OTAI_OBJECT_TYPE_LINECARD, m_linecard_rid, 1, &attr);
        if (status != OTAI_STATUS_SUCCESS)
        {
            continue;
        }
        if (mode == attr.value.chardata)
        {
            break;
        }
    } while (wait_count < 10 * 60); /* 10 minutes is enough for OTN to change its boardmode */

    SWSS_LOG_NOTICE("The end of setting board-mode");
}

void SoftReiniter::processLinecards()
{
    SWSS_LOG_ENTER();

    if (m_linecards.size() > 1) {
        SWSS_LOG_THROW("multiple linecards %zu in single hard reinit are not allowed", m_linecards.size());
    }

    for (const auto& s : m_linecards) {
        std::string strLinecardVid = s.first;
        std::string asicKey = s.second;

        otai_deserialize_object_id(strLinecardVid, m_linecard_vid); 
        if (m_linecard_vid == OTAI_NULL_OBJECT_ID) {
            SWSS_LOG_THROW("linecard id can't be NULL");
        }
        auto oit = m_oids.find(strLinecardVid);
        if (oit == m_oids.end()) {
            SWSS_LOG_THROW("failed to find VID %s in OIDs map", strLinecardVid.c_str());
        }
        
        std::shared_ptr<OtaiAttributeList> list = m_attributesLists[asicKey];
        otai_attribute_t* attrList = list->get_attr_list();
        uint32_t attr_count = 0;
        std::vector<otai_attribute_t> attrs;

        bool is_board_mode_existed = false;
        std::string board_mode;

        for (uint32_t idx = 0 ; idx < list->get_attr_count(); ++idx) {
            auto meta = otai_metadata_get_attr_metadata(OTAI_OBJECT_TYPE_LINECARD, attrList[idx].id);
            SWSS_LOG_NOTICE("process, attr_id=%s", otai_serialize_attr_id(*meta).c_str());
            if (attrList[idx].id == OTAI_LINECARD_ATTR_BOARD_MODE) {
                is_board_mode_existed = true;
                board_mode = (attrList[idx].value.chardata);
            } else if (!OTAI_HAS_FLAG_CREATE_ONLY(meta->flags)) {
                attrs.push_back(attrList[idx]);
                attr_count++;
            }
        }

        otai_status_t status;
        m_linecard_rid = m_vidToRidMap[m_linecard_vid];

        m_translatedV2R[m_linecard_vid] = m_linecard_rid;
        m_translatedR2V[m_linecard_rid] = m_linecard_vid;

        otai_attribute_t pre_config_attr;
        pre_config_attr.id = OTAI_LINECARD_ATTR_START_PRE_CONFIGURATION;
        pre_config_attr.value.booldata = true;
       
        SWSS_LOG_NOTICE("set linecard attr, attr_id=OTAI_LINECARD_ATTR_START_PRE_CONFIGURATION");
        status = m_vendorOtai->set(OTAI_OBJECT_TYPE_LINECARD, m_linecard_rid, &pre_config_attr);
        if (status != OTAI_STATUS_SUCCESS) {
            SWSS_LOG_THROW("failed to start pre-config linecard");
        }

        if (is_board_mode_existed) {
            setBoardMode(board_mode);
        }

        for (uint32_t idx = 0; idx < attr_count; ++idx) {
            otai_attribute_t* attr = &attrs[idx];
            auto meta = otai_metadata_get_attr_metadata(OTAI_OBJECT_TYPE_LINECARD, attr->id);
            if (meta == NULL) {
                SWSS_LOG_THROW("failed to get Linecard metadata, attr=%d", attr->id);
                continue;
            }
            SWSS_LOG_NOTICE("set linecard attr, attr_id=%s", otai_serialize_attr_id(*meta).c_str());

            status = m_vendorOtai->set(OTAI_OBJECT_TYPE_LINECARD, m_linecard_rid, attr);
            if (status != OTAI_STATUS_SUCCESS) {
                SWSS_LOG_THROW("failed to set attribute %s on linecard VID %s: %s",
                    otai_metadata_get_attr_metadata(OTAI_OBJECT_TYPE_LINECARD, attr->id)->attridname,
                    otai_serialize_object_id(m_linecard_rid).c_str(),
                    otai_serialize_status(status).c_str());
            }
        } 
    }
}

otai_object_id_t SoftReiniter::processSingleVid(_In_ otai_object_id_t vid)
{
    SWSS_LOG_ENTER();

    if (vid == OTAI_NULL_OBJECT_ID) {
         SWSS_LOG_DEBUG("processed VID 0 to RID 0");
         return OTAI_NULL_OBJECT_ID;
    }

    auto it = m_translatedV2R.find(vid);
    if (it != m_translatedV2R.end()) {
        return it->second;
    }

    otai_object_type_t objectType = VidManager::objectTypeQuery(vid);
    std::string strVid = otai_serialize_object_id(vid);
   
    auto oit = m_oids.find(strVid);
    if (oit == m_oids.end()) {
        SWSS_LOG_THROW("failed to find VID %s in OIDs map", strVid.c_str());
    }
    std::string asicKey = oit->second;
    std::shared_ptr<OtaiAttributeList> list = m_attributesLists[asicKey];
    otai_attribute_t* attrList = list->get_attr_list();
    uint32_t attrCount = list->get_attr_count();
    auto v2rMapIt = m_vidToRidMap.find(vid);
    if (v2rMapIt == m_vidToRidMap.end()) {
        SWSS_LOG_THROW("failed to find VID %s in VIDTORID map",
            otai_serialize_object_id(vid).c_str());
    }
    otai_object_id_t rid;
    rid = v2rMapIt->second;
   // uint32_t attr_count = 0;
    uint32_t attr_count_left = 0;
   // std::vector<otai_attribute_t> attrs;
    std::vector<otai_attribute_t> attrs_left; 
    for (uint32_t idx = 0; idx < attrCount; ++idx) {
        auto meta = otai_metadata_get_attr_metadata(objectType, attrList[idx].id);
        if (!OTAI_HAS_FLAG_CREATE_ONLY(meta->flags)) {
            attrs_left.push_back(attrList[idx]);
            attr_count_left++;
        }
    }
    SWSS_LOG_DEBUG("setting attributes on object of type %x, processed VID 0x%" PRIx64 " to RID 0x%" PRIx64 " ", objectType, vid, rid);
    for (uint32_t idx = 0; idx < attr_count_left; idx++) {
        otai_attribute_t* attr = &attrs_left[idx];
        auto meta = otai_metadata_get_attr_metadata(objectType, attr->id);
        if (meta == NULL) {
            SWSS_LOG_THROW("failed to get attribute metadata %s: %d",
                otai_serialize_object_type(objectType).c_str(),
                attr->id);
        }
        SWSS_LOG_NOTICE("set %s attr, attr_id=%s", otai_serialize_object_type(objectType).c_str(), otai_serialize_attr_id(*meta).c_str());
        otai_status_t status = m_vendorOtai->set(objectType, rid, attr);
        if (status != OTAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR(
                "failed to set %s value %s: %s",
                meta->attridname,
                otai_serialize_attr_value(*meta, *attr).c_str(),
                otai_serialize_status(status).c_str());
        }
    }
    m_translatedV2R[vid] = rid;
    m_translatedR2V[rid] = vid;

    return rid;
}

void SoftReiniter::processOids()
{
    SWSS_LOG_ENTER();

    for (const auto &kv: m_oids) {
        const std::string& strObjectId = kv.first;

        otai_object_id_t vid;
        otai_deserialize_object_id(strObjectId, vid);
        processSingleVid(vid);
    }
}

void SoftReiniter::softReinit()
{
    SWSS_LOG_ENTER();

    readAsicState();

    for (auto& kvp: m_linecardMap) {
        prepareAsicState(kvp.second); 
        processLinecards();
        processOids();
        stopPreConfigLinecards();
    }

    auto flexCounterGroupKeys = m_client->getFlexCounterGroupKeys();
    auto flexCounterKeys = m_client->getFlexCounterKeys();

    auto fgr = std::make_shared<FlexCounterGroupReiniter>(
                m_client,
                m_manager,
                flexCounterGroupKeys);

    fgr->hardReinit();

    for (auto& kvp: m_linecardMap)
    {
        auto fr = std::make_shared<FlexCounterReiniter>(
                m_client,
                m_translator,
                m_manager,
                m_linecardVidToRid.at(kvp.first),
                m_linecardRidToVid.at(kvp.first),
                flexCounterKeys);
        fr->hardReinit();
    }

}

otai_object_type_t SoftReiniter::getObjectTypeFromAsicKey(
    _In_ const string& key)
{
    SWSS_LOG_ENTER();

    auto start = key.find_first_of(":") + 1;
    auto end = key.find(":", start);

    const string strObjectType = key.substr(start, end - start);

    otai_object_type_t objectType;
    otai_deserialize_object_type(strObjectType, objectType);

    if (!otai_metadata_is_object_type_valid(objectType))
    {   
        SWSS_LOG_THROW("invalid object type: %s on asic key: %s",
            otai_serialize_object_type(objectType).c_str(),
            key.c_str());
    }   

    return objectType;
}

string SoftReiniter::getObjectIdFromAsicKey(
    _In_ const string& key)
{
    SWSS_LOG_ENTER();

    auto start = key.find_first_of(":") + 1;
    auto end = key.find(":", start);

    return key.substr(end + 1);
}

shared_ptr<otaimeta::OtaiAttributeList> SoftReiniter::redisGetAttributesFromAsicKey(
    _In_ const string& key)
{
    SWSS_LOG_ENTER();

    otai_object_type_t objectType = getObjectTypeFromAsicKey(key);

    vector<swss::FieldValueTuple> values;

    auto hash = m_client->getAttributesFromAsicKey(key);
    for (auto& kv : hash) {
        const string& skey = kv.first;
        const string& svalue = kv.second;
        swss::FieldValueTuple fvt(skey, svalue);
        values.push_back(fvt);
    }
    return make_shared<otaimeta::OtaiAttributeList>(objectType, values, false);
}

