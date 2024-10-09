#include <string>
#include <memory>  
#include <fstream>
#include "OtaiObjectSimulator.h"
#include "RealObjectIdManager.h"
#include "otaistatus.h"
#include "swss/logger.h"

#include "meta/otai_serialize.h"

using namespace std;
using namespace otaivs;
using json = nlohmann::json;

const string sim_data_path = "/usr/include/vslib/otai_sim_data/";

static std::shared_ptr<OtaiObjectSimulator> g_OtaiObjSims[OTAI_OBJECT_TYPE_MAX];
bool OtaiObjectSimulator::isObjectSimulatorInitialed = false;

std::shared_ptr<OtaiObjectSimulator> OtaiObjectSimulator::getOtaiObjectSimulator(
    _In_ otai_object_type_t object_type) 
{
    if(!isObjectSimulatorInitialed) {
        for(int i = OTAI_OBJECT_TYPE_LINECARD; i<OTAI_OBJECT_TYPE_MAX; i++) {
            g_OtaiObjSims[i] = make_shared<OtaiObjectSimulator>((otai_object_type_t)i);
        }
        isObjectSimulatorInitialed = true;
    }

    return g_OtaiObjSims[(int)object_type];
}


OtaiObjectSimulator::OtaiObjectSimulator(
    _In_ otai_object_type_t object_type) :
    m_objectType(object_type)
{
    string objType = otai_serialize_object_type(object_type);
    string shortName = objType.erase(0, strlen("OTAI_OBJECT_TYPE_"));
    transform(shortName.begin(), shortName.end(), shortName.begin(), ::tolower); 
    string fileName = "otai_" + shortName + "_sim.json";
    string sim_data_file = sim_data_path + fileName;

    std::ifstream ifs(sim_data_file);
    if (!ifs.good())
    {
        SWSS_LOG_ERROR("OtaiObjectSimulator failed to read '%s', err: %s", sim_data_file, strerror(errno));
        return;
    }

    try
    {
        m_data = json::parse(ifs);
    }
    catch (const std::exception& e)
    {
        SWSS_LOG_ERROR("OtaiObjectSimulator Failed to parse '%s': %s", sim_data_path, e.what());
    }
}

otai_status_t OtaiObjectSimulator::create(
    _Out_ otai_object_id_t* objectId,
    _In_ otai_object_id_t linecardId,
    _In_ uint32_t attr_count,
    _In_ const otai_attribute_t *attr_list) 
{
    SWSS_LOG_ENTER();

    if (m_objectType == OTAI_OBJECT_TYPE_LINECARD)
    {
        *objectId = RealObjectIdManager::allocateNewLinecardObjectId();
    }
    else
    {
        *objectId = RealObjectIdManager::allocateNewObjectId(m_objectType, linecardId, attr_count, attr_list);
    }

    SWSS_LOG_NOTICE("Calling OtaiObjectSimulator::create type:%s, rid:%s",
            otai_serialize_object_type(m_objectType).c_str(), 
            otai_serialize_object_id(*objectId).c_str());

    return OTAI_STATUS_SUCCESS;
}

otai_status_t OtaiObjectSimulator::remove(
    _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();
    SWSS_LOG_NOTICE("Calling OtaiObjectSimulator::remove type:%s, rid:%s",
            otai_serialize_object_type(m_objectType).c_str(), 
            otai_serialize_object_id(objectId).c_str());

    return OTAI_STATUS_SUCCESS;
}

otai_status_t OtaiObjectSimulator::set(
    _In_ otai_object_id_t objectId,
    _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();
    auto meta = otai_metadata_get_attr_metadata(m_objectType, attr->id);

    if (meta == NULL)
    {
        SWSS_LOG_ERROR("failed to find attribute %d for %s:%s", attr->id,
                otai_serialize_object_type(m_objectType).c_str(),
                otai_serialize_object_id(objectId).c_str());
        return OTAI_STATUS_FAILURE;
    }

    string attriName(meta->attridname);
    SWSS_LOG_NOTICE("Calling OtaiObjectSimulator::set type: %s, rid: %s, attr: %s",
            otai_serialize_object_type(m_objectType).c_str(), 
            otai_serialize_object_id(objectId).c_str(),
            attriName.c_str());

    switch (meta->attrvaluetype)
    {
        case OTAI_ATTR_VALUE_TYPE_BOOL:
            m_data[attriName] = attr->value.booldata;
            break;
        case OTAI_ATTR_VALUE_TYPE_UINT8:
            m_data[attriName] = attr->value.u8;
            break;
        case OTAI_ATTR_VALUE_TYPE_INT8:
            m_data[attriName] = attr->value.s8;
            break;
        case OTAI_ATTR_VALUE_TYPE_UINT16:
            m_data[attriName] = attr->value.u16;
            break;
        case OTAI_ATTR_VALUE_TYPE_INT16:
            m_data[attriName] = attr->value.s16;
            break;
        case OTAI_ATTR_VALUE_TYPE_UINT32:
            m_data[attriName] = attr->value.u32;
            break;
        case OTAI_ATTR_VALUE_TYPE_INT32:
            m_data[attriName] = attr->value.s32;
            break;
        case OTAI_ATTR_VALUE_TYPE_UINT64:
            m_data[attriName] = attr->value.u64;
            break;
        case OTAI_ATTR_VALUE_TYPE_INT64:
            m_data[attriName] = attr->value.s64;
            break;
        case OTAI_ATTR_VALUE_TYPE_DOUBLE:
            m_data[attriName] = attr->value.d64;
            break;
        case OTAI_ATTR_VALUE_TYPE_CHARDATA:
            m_data[attriName] = string(attr->value.chardata);
            break;
        default:
            break;
    }

    return OTAI_STATUS_SUCCESS;
} 

otai_status_t OtaiObjectSimulator::get(
    _In_ otai_object_id_t objectId,
    _In_ uint32_t attr_count,
    _Inout_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    string serializedObjectId =  otai_serialize_object_id(objectId);
    for (uint32_t idx = 0; idx < attr_count; ++idx)
    {
        otai_attr_id_t id = attr_list[idx].id;

        auto meta = otai_metadata_get_attr_metadata(m_objectType, id);
        if (meta == NULL)
        {
            SWSS_LOG_ERROR("failed to find attribute %d for %s:%s", id,
                    otai_serialize_object_type(m_objectType).c_str(),
                    serializedObjectId.c_str());

            return OTAI_STATUS_FAILURE;
        }

        string attriName(meta->attridname);
        SWSS_LOG_DEBUG("Calling OtaiObjectSimulator::get type: %s, rid: %s, attr: %s",
                otai_serialize_object_type(m_objectType).c_str(), 
                otai_serialize_object_id(objectId).c_str(),
                attriName.c_str());

        switch (meta->attrvaluetype)
        {
            case OTAI_ATTR_VALUE_TYPE_BOOL:
                attr_list[idx].value.booldata = m_data[attriName];
                break;
            case OTAI_ATTR_VALUE_TYPE_UINT8:
                attr_list[idx].value.u8 = m_data[attriName];
                break;
            case OTAI_ATTR_VALUE_TYPE_INT8:
                attr_list[idx].value.s8 = m_data[attriName];
                break;
            case OTAI_ATTR_VALUE_TYPE_UINT16:
                attr_list[idx].value.u16 = m_data[attriName];
                break;
            case OTAI_ATTR_VALUE_TYPE_INT16:
                attr_list[idx].value.s16 = m_data[attriName];
                break;
            case OTAI_ATTR_VALUE_TYPE_UINT32:
                attr_list[idx].value.u32 = m_data[attriName];
                break;
            case OTAI_ATTR_VALUE_TYPE_INT32:
                attr_list[idx].value.s32 = m_data[attriName];
                break;
            case OTAI_ATTR_VALUE_TYPE_UINT64:
                attr_list[idx].value.u64 = m_data[attriName];
                break;
            case OTAI_ATTR_VALUE_TYPE_INT64:
                attr_list[idx].value.s64 = m_data[attriName];
                break;
            case OTAI_ATTR_VALUE_TYPE_DOUBLE:
                attr_list[idx].value.d64 = m_data[attriName];
                break;
            case OTAI_ATTR_VALUE_TYPE_CHARDATA:
                strncpy(attr_list[idx].value.chardata, ((string)m_data[attriName]).c_str(), sizeof(attr_list[idx].value.chardata));
                break;
            default:
                break;
        }

        SWSS_LOG_DEBUG("Finish OtaiObjectSimulator::get type: %s, rid: %s, attr: %s",
                otai_serialize_object_type(m_objectType).c_str(), 
                otai_serialize_object_id(objectId).c_str(),
                attriName.c_str());
    }

    return OTAI_STATUS_SUCCESS;
}

otai_status_t OtaiObjectSimulator::getStatsExt(
    _In_ otai_object_id_t object_id,
    _In_ uint32_t number_of_counters,
    _In_ const otai_stat_id_t *counter_ids,
    _In_ otai_stats_mode_t mode,
    _Out_ otai_stat_value_t *counters)
{
    SWSS_LOG_ENTER();
    
    for (uint32_t i = 0; i < number_of_counters; ++i)
    {
        int32_t id = counter_ids[i];
        auto stat_metadata  = otai_metadata_get_stat_metadata(m_objectType, id);
        string statidname = stat_metadata->statidname;

        SWSS_LOG_DEBUG("Calling OtaiObjectSimulator::getStatsExt type: %s, rid: %s, stat: %s",
            otai_serialize_object_type(m_objectType).c_str(), 
            otai_serialize_object_id(object_id).c_str(),
            statidname.c_str());
        
        if (stat_metadata->statvaluetype == OTAI_STAT_VALUE_TYPE_UINT64) {
            counters[i].u64 = m_data[statidname];
            if (mode == OTAI_STATS_MODE_READ_AND_CLEAR)
            {
                counters[i].u64 = 0;
            }
        } else if (stat_metadata->statvaluetype == OTAI_STAT_VALUE_TYPE_DOUBLE) {
            counters[i].d64 = m_data[statidname];
        }

        SWSS_LOG_DEBUG("finish OtaiObjectSimulator::getStatsExt type: %s, rid: %s, stat: %s",
            otai_serialize_object_type(m_objectType).c_str(), 
            otai_serialize_object_id(object_id).c_str(),
            statidname.c_str());
    }

    return OTAI_STATUS_SUCCESS;
}

otai_status_t OtaiObjectSimulator::clearStats(
        _In_ otai_object_id_t object_id,
        _In_ uint32_t number_of_counters,
        _In_ const otai_stat_id_t *counter_ids)
{
    SWSS_LOG_ENTER();
    SWSS_LOG_NOTICE("Calling OtaiObjectSimulator::clearStats type:%s, rid:%s",
            otai_serialize_object_type(m_objectType).c_str(), 
            otai_serialize_object_id(object_id).c_str());
    
    return OTAI_STATUS_SUCCESS; 
}