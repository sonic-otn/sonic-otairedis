#include <thread>
#include <chrono>
#include <inttypes.h>

#include "SoftReiniter.h"
#include "FlexCounterReiniter.h"
#include "VidManager.h"
#include "RedisClient.h"
#include "swss/logger.h"
#include "meta/lai_serialize.h"

using namespace syncd;
using namespace laimeta;
using namespace std;

SoftReiniter::SoftReiniter(
    _In_ shared_ptr<RedisClient> client,
    _In_ std::shared_ptr<VirtualOidTranslator> translator,
    _In_ std::shared_ptr<lairedis::LaiInterface> lai,
    _In_ std::shared_ptr<FlexCounterManager> manager):
    m_vendorLai(lai),
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

        lai_object_meta_key_t metaKey;
        lai_deserialize_object_meta_key(mk, metaKey);

        // if object is non object id then first item will be linecard id
        auto linecardId = VidManager::linecardIdQuery(metaKey.objectkey.key.object_id);

        m_linecardMap[linecardId].push_back(key);
    }   
    SWSS_LOG_NOTICE("loaded %zu linecards", m_linecardMap.size());

    for (auto& kvp: m_linecardMap) {
        SWSS_LOG_NOTICE("linecard VID: %s keys %d", lai_serialize_object_id(kvp.first).c_str(), kvp.second.size());
    }
}

void SoftReiniter::prepareAsicState(vector<string> &asicKeys)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_TIMER("read asic state asicKeys %d", (int)asicKeys.size());

    for (auto &key : asicKeys) {
        lai_object_type_t objectType = getObjectTypeFromAsicKey(key);
        const string& strObjectId = getObjectIdFromAsicKey(key);
        string strObjectType = lai_serialize_object_type(objectType);

        SWSS_LOG_NOTICE("objectType = %s, objectId=%s", strObjectType.c_str(), strObjectId.c_str());

        auto info = lai_metadata_get_object_type_info(objectType);
        switch (objectType) {
        case LAI_OBJECT_TYPE_LINECARD:
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

        lai_deserialize_object_id(strLinecardVid, m_linecard_vid); 
        if (m_linecard_vid == LAI_NULL_OBJECT_ID) {
            SWSS_LOG_THROW("linecard id can't be NULL");
        }
        auto oit = m_oids.find(strLinecardVid);
        if (oit == m_oids.end()) {
            SWSS_LOG_THROW("failed to find VID %s in OIDs map", strLinecardVid.c_str());
        }
        m_linecard_rid = m_vidToRidMap[m_linecard_vid];

        lai_attribute_t attr;
        attr.id = LAI_LINECARD_ATTR_STOP_PRE_CONFIGURATION;
        attr.value.booldata = true;

        SWSS_LOG_NOTICE("Stop pre-config linecard");

        lai_status_t status = m_vendorLai->set(LAI_OBJECT_TYPE_LINECARD, m_linecard_rid, &attr);
        if (status != LAI_STATUS_SUCCESS) {
            SWSS_LOG_THROW("failed to stop pre-config linecard");
        }
    }
}

void SoftReiniter::setBoardMode(lai_linecard_board_mode_t mode)
{
    SWSS_LOG_ENTER();

    int wait_count = 0;
    lai_attribute_t attr;
    lai_status_t status;

    attr.id = LAI_LINECARD_ATTR_BOARD_MODE;
    status = m_vendorLai->get(LAI_OBJECT_TYPE_LINECARD, m_linecard_rid, 1, &attr);
    if (status == LAI_STATUS_SUCCESS && attr.value.s32 == mode)
    {   
        SWSS_LOG_DEBUG("Linecard and maincard have a same board-mode, %d", mode);
        return;
    }   

    SWSS_LOG_NOTICE("Begin to set board-mode %d", mode);

    attr.value.s32 = mode;
    status = m_vendorLai->set(LAI_OBJECT_TYPE_LINECARD, m_linecard_rid, &attr);
    if (status != LAI_STATUS_SUCCESS)
    {   
        SWSS_LOG_ERROR("Failed to set board-mode status=%d, mode=%d",
                       status, mode);
        return;
    }
    do
    {
        wait_count++;
        this_thread::sleep_for(chrono::milliseconds(1000));
        status = m_vendorLai->get(LAI_OBJECT_TYPE_LINECARD, m_linecard_rid, 1, &attr);
        if (status != LAI_STATUS_SUCCESS)
        {
            continue;
        }
        if (attr.value.s32 == mode)
        {
            break;
        }
    } while (wait_count < 10 * 60); /* 10 minutes is enough for P230C to change its boardmode */

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

        lai_deserialize_object_id(strLinecardVid, m_linecard_vid); 
        if (m_linecard_vid == LAI_NULL_OBJECT_ID) {
            SWSS_LOG_THROW("linecard id can't be NULL");
        }
        auto oit = m_oids.find(strLinecardVid);
        if (oit == m_oids.end()) {
            SWSS_LOG_THROW("failed to find VID %s in OIDs map", strLinecardVid.c_str());
        }
        
        std::shared_ptr<LaiAttributeList> list = m_attributesLists[asicKey];
        lai_attribute_t* attrList = list->get_attr_list();
        uint32_t attr_count = 0;
        std::vector<lai_attribute_t> attrs;

        bool is_board_mode_existed = false;
        lai_linecard_board_mode_t board_mode = LAI_LINECARD_BOARD_MODE_L1_400G_CA_100GE;

        for (uint32_t idx = 0 ; idx < list->get_attr_count(); ++idx) {
            auto meta = lai_metadata_get_attr_metadata(LAI_OBJECT_TYPE_LINECARD, attrList[idx].id);
            SWSS_LOG_NOTICE("process, attr_id=%s", lai_serialize_attr_id(*meta).c_str());
            if (attrList[idx].id == LAI_LINECARD_ATTR_BOARD_MODE) {
                is_board_mode_existed = true;
                board_mode = (lai_linecard_board_mode_t)(attrList[idx].value.s32);
            } else if (!LAI_HAS_FLAG_CREATE_ONLY(meta->flags)) {
                attrs.push_back(attrList[idx]);
                attr_count++;
            }
        }

        lai_status_t status;
        m_linecard_rid = m_vidToRidMap[m_linecard_vid];

        m_translatedV2R[m_linecard_vid] = m_linecard_rid;
        m_translatedR2V[m_linecard_rid] = m_linecard_vid;

        lai_attribute_t pre_config_attr;
        pre_config_attr.id = LAI_LINECARD_ATTR_START_PRE_CONFIGURATION;
        pre_config_attr.value.booldata = true;
       
        SWSS_LOG_NOTICE("set linecard attr, attr_id=LAI_LINECARD_ATTR_START_PRE_CONFIGURATION");
        status = m_vendorLai->set(LAI_OBJECT_TYPE_LINECARD, m_linecard_rid, &pre_config_attr);
        if (status != LAI_STATUS_SUCCESS) {
            SWSS_LOG_THROW("failed to start pre-config linecard");
        }

        if (is_board_mode_existed) {
            setBoardMode(board_mode);
        }

        for (uint32_t idx = 0; idx < attr_count; ++idx) {
            lai_attribute_t* attr = &attrs[idx];
            auto meta = lai_metadata_get_attr_metadata(LAI_OBJECT_TYPE_LINECARD, attr->id);
            if (meta == NULL) {
                SWSS_LOG_THROW("failed to get Linecard metadata, attr=%d", attr->id);
                continue;
            }
            SWSS_LOG_NOTICE("set linecard attr, attr_id=%s", lai_serialize_attr_id(*meta).c_str());

            status = m_vendorLai->set(LAI_OBJECT_TYPE_LINECARD, m_linecard_rid, attr);
            if (status != LAI_STATUS_SUCCESS) {
                SWSS_LOG_THROW("failed to set attribute %s on linecard VID %s: %s",
                    lai_metadata_get_attr_metadata(LAI_OBJECT_TYPE_LINECARD, attr->id)->attridname,
                    lai_serialize_object_id(m_linecard_rid).c_str(),
                    lai_serialize_status(status).c_str());
            }
        } 
    }
}

lai_object_id_t SoftReiniter::processSingleVid(_In_ lai_object_id_t vid)
{
    SWSS_LOG_ENTER();

    if (vid == LAI_NULL_OBJECT_ID) {
         SWSS_LOG_DEBUG("processed VID 0 to RID 0");
         return LAI_NULL_OBJECT_ID;
    }

    auto it = m_translatedV2R.find(vid);
    if (it != m_translatedV2R.end()) {
        return it->second;
    }

    lai_object_type_t objectType = VidManager::objectTypeQuery(vid);
    std::string strVid = lai_serialize_object_id(vid);
   
    auto oit = m_oids.find(strVid);
    if (oit == m_oids.end()) {
        SWSS_LOG_THROW("failed to find VID %s in OIDs map", strVid.c_str());
    }
    std::string asicKey = oit->second;
    std::shared_ptr<LaiAttributeList> list = m_attributesLists[asicKey];
    lai_attribute_t* attrList = list->get_attr_list();
    uint32_t attrCount = list->get_attr_count();
    auto v2rMapIt = m_vidToRidMap.find(vid);
    if (v2rMapIt == m_vidToRidMap.end()) {
        SWSS_LOG_THROW("failed to find VID %s in VIDTORID map",
            lai_serialize_object_id(vid).c_str());
    }
    lai_object_id_t rid;
    rid = v2rMapIt->second;
   // uint32_t attr_count = 0;
    uint32_t attr_count_left = 0;
   // std::vector<lai_attribute_t> attrs;
    std::vector<lai_attribute_t> attrs_left; 
    for (uint32_t idx = 0; idx < attrCount; ++idx) {
        auto meta = lai_metadata_get_attr_metadata(objectType, attrList[idx].id);
        if (!LAI_HAS_FLAG_CREATE_ONLY(meta->flags)) {
            attrs_left.push_back(attrList[idx]);
            attr_count_left++;
        }
    }
    SWSS_LOG_DEBUG("setting attributes on object of type %x, processed VID 0x%" PRIx64 " to RID 0x%" PRIx64 " ", objectType, vid, rid);
    for (uint32_t idx = 0; idx < attr_count_left; idx++) {
        lai_attribute_t* attr = &attrs_left[idx];
        auto meta = lai_metadata_get_attr_metadata(objectType, attr->id);
        if (meta == NULL) {
            SWSS_LOG_THROW("failed to get attribute metadata %s: %d",
                lai_serialize_object_type(objectType).c_str(),
                attr->id);
        }
        SWSS_LOG_NOTICE("set %s attr, attr_id=%s", lai_serialize_object_type(objectType).c_str(), lai_serialize_attr_id(*meta).c_str());
        lai_status_t status = m_vendorLai->set(objectType, rid, attr);
        if (status != LAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR(
                "failed to set %s value %s: %s",
                meta->attridname,
                lai_serialize_attr_value(*meta, *attr).c_str(),
                lai_serialize_status(status).c_str());
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

        lai_object_id_t vid;
        lai_deserialize_object_id(strObjectId, vid);
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

lai_object_type_t SoftReiniter::getObjectTypeFromAsicKey(
    _In_ const string& key)
{
    SWSS_LOG_ENTER();

    auto start = key.find_first_of(":") + 1;
    auto end = key.find(":", start);

    const string strObjectType = key.substr(start, end - start);

    lai_object_type_t objectType;
    lai_deserialize_object_type(strObjectType, objectType);

    if (!lai_metadata_is_object_type_valid(objectType))
    {   
        SWSS_LOG_THROW("invalid object type: %s on asic key: %s",
            lai_serialize_object_type(objectType).c_str(),
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

shared_ptr<laimeta::LaiAttributeList> SoftReiniter::redisGetAttributesFromAsicKey(
    _In_ const string& key)
{
    SWSS_LOG_ENTER();

    lai_object_type_t objectType = getObjectTypeFromAsicKey(key);

    vector<swss::FieldValueTuple> values;

    auto hash = m_client->getAttributesFromAsicKey(key);
    for (auto& kv : hash) {
        const string& skey = kv.first;
        const string& svalue = kv.second;
        swss::FieldValueTuple fvt(skey, svalue);
        values.push_back(fvt);
    }
    return make_shared<laimeta::LaiAttributeList>(objectType, values, false);
}

