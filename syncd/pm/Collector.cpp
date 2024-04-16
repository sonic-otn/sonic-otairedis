/**
 * Copyright (c) 2023 Alibaba Group Holding Limited
 * Copyright (c) 2023 Accelink Technologies Co., Ltd.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License"); you may
 *    not use this file except in compliance with the License. You may obtain
 *    a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 *    THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 *    CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *    LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 *    FOR A PARTICULAR PURPOSE, MERCHANTABILITY OR NON-INFRINGEMENT.
 *
 *    See the Apache Version 2.0 License for specific language governing
 *    permissions and limitations under the License.
 *
 */

#include <algorithm>
#include <math.h>

#include "Collector.h"

using namespace std;
using namespace syncd;

Collector::Collector(
    _In_ otai_object_type_t objectType,
    _In_ otai_object_id_t vid,
    _In_ otai_object_id_t rid,
    std::shared_ptr<otairedis::OtaiInterface> vendorOtai) :
    m_objectType(objectType),
    m_vid(vid),
    m_rid(rid),
    m_vendorOtai(vendorOtai)
{
    SWSS_LOG_ENTER();

    m_stateDb = shared_ptr<swss::DBConnector>(new swss::DBConnector("STATE_DB", 0));
    m_countersDb = shared_ptr<swss::DBConnector>(new swss::DBConnector("COUNTERS_DB", 0));
    m_historyDb = shared_ptr<swss::DBConnector>(new swss::DBConnector(HISTORY_DB_NAME, 0));

    string strStateTable;
    string strCountersTable;
    string strTableNameMap;

    switch (objectType)
    {
    case OTAI_OBJECT_TYPE_LINECARD:
        strStateTable = STATE_OT_LINECARD_TABLE_NAME;
        strCountersTable = COUNTERS_OT_LINECARD_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_LINECARD_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_PORT:
        strStateTable = STATE_OT_PORT_TABLE_NAME;
        strCountersTable = COUNTERS_OT_PORT_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_PORT_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_TRANSCEIVER:
        strStateTable = STATE_OT_TRANSCEIVER_TABLE_NAME;
        strCountersTable = COUNTERS_OT_TRANSCEIVER_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_TRANSCEIVER_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_LOGICALCHANNEL:
        strStateTable = STATE_OT_LOGICALCHANNEL_TABLE_NAME;
        strCountersTable = COUNTERS_OT_LOGICALCHANNEL_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_LOGICALCHANNEL_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_OTN:
        strStateTable = STATE_OT_OTN_TABLE_NAME;
        strCountersTable = COUNTERS_OT_OTN_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_OTN_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_ETHERNET:
        strStateTable = STATE_OT_ETHERNET_TABLE_NAME;
        strCountersTable = COUNTERS_OT_ETHERNET_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_ETHERNET_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_PHYSICALCHANNEL:
        strStateTable = COUNTERS_OT_PHYSICALCHANNEL_TABLE_NAME;
        strCountersTable = COUNTERS_OT_PHYSICALCHANNEL_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_PHYSICALCHANNEL_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_OCH:
        strStateTable = STATE_OT_OCH_TABLE_NAME;
        strCountersTable = COUNTERS_OT_OCH_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_OCH_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_LLDP:
        strStateTable = STATE_OT_LLDP_TABLE_NAME;
        strCountersTable = COUNTERS_OT_LLDP_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_LLDP_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_ASSIGNMENT:
        strStateTable = STATE_OT_ASSIGNMENT_TABLE_NAME;
        strCountersTable = COUNTERS_OT_ASSIGNMENT_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_ASSIGNMENT_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_INTERFACE:
        strStateTable = STATE_OT_INTERFACE_TABLE_NAME;
        strCountersTable = COUNTERS_OT_INTERFACE_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_INTERFACE_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_OA:
        strStateTable = STATE_OT_OA_TABLE_NAME;
        strCountersTable = COUNTERS_OT_OA_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_OA_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_OSC:
        strStateTable = STATE_OT_OSC_TABLE_NAME;
        strCountersTable = COUNTERS_OT_OSC_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_OSC_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_APS:
        strStateTable = STATE_OT_APS_TABLE_NAME;
        strCountersTable = COUNTERS_OT_APS_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_APS_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_APSPORT:
        strStateTable = STATE_OT_APSPORT_TABLE_NAME;
        strCountersTable = COUNTERS_OT_APSPORT_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_APSPORT_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_ATTENUATOR:
        strStateTable = STATE_OT_ATTENUATOR_TABLE_NAME;
        strCountersTable = COUNTERS_OT_ATTENUATOR_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_ATTENUATOR_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_OCM:
        strStateTable = STATE_OT_OCM_TABLE_NAME;
        strCountersTable = COUNTERS_OT_OCM_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_OCM_NAME_MAP;
        break;
    case OTAI_OBJECT_TYPE_OTDR:
        strStateTable = STATE_OT_OTDR_TABLE_NAME;
        strCountersTable = COUNTERS_OT_OTDR_TABLE_NAME;
        strTableNameMap = COUNTERS_OT_OTDR_NAME_MAP;
        break;
    default:
        SWSS_LOG_THROW("Unsupported object type:%d", objectType);
    }
    
    m_stateTable = unique_ptr<swss::Table>(new swss::Table(m_stateDb.get(), strStateTable));
    m_countersTable = unique_ptr<swss::Table>(new swss::Table(m_countersDb.get(), strCountersTable));
    m_historyTable = unique_ptr<swss::Table>(new swss::Table(m_historyDb.get(), strCountersTable));

    m_countersTableName = strCountersTable;

    swss::DBConnector dbCounters("COUNTERS_DB", 0); 
    std::string strVid = otai_serialize_object_id(vid);
    auto key = dbCounters.hget(strTableNameMap, strVid);
    if (key != NULL)
    {
        m_stateTableKeyName = *key;
    }
    else
    {
        m_stateTableKeyName = ""; 
        SWSS_LOG_ERROR("Cann't get name map, tableNameMap:%s, vid:%s",
                       strTableNameMap.c_str(), strVid.c_str());
    }

    m_countersTableKeyName = m_stateTableKeyName;
    replace(m_countersTableKeyName.begin(), m_countersTableKeyName.end(), '|', ':');

    m_historyTableKeyName = m_countersTableKeyName;

    m_counter15min = 0;
    m_counter24hour = 0;
}

Collector::~Collector()
{
    SWSS_LOG_ENTER();
}

void Collector::updateTimeFlags()
{
    SWSS_LOG_ENTER();

    m_timeout15min = false;
    m_timeout24hour = false;

    const auto p = chrono::system_clock::now().time_since_epoch();

    auto hours_count = chrono::duration_cast<chrono::hours>(p).count();

    auto minutes_count = chrono::duration_cast<chrono::minutes>(p).count();

    auto nanoseconds_count = chrono::duration_cast<chrono::nanoseconds>(p).count();

    m_collectTime = nanoseconds_count;

    if ((m_counter15min == 0) ||
        ((minutes_count % 15) == 0 && (minutes_count / 15) > m_counter15min))
    {
        m_timeout15min = true;
        m_counter15min = minutes_count / 15;
    }
 
    if ((m_counter24hour == 0) ||
        ((hours_count % 24) == 0 && (hours_count / 24) > m_counter24hour))
    {
        m_timeout24hour = true;
        m_counter24hour = hours_count / 24;
    }
}

double Collector::convertMilliWatt2dBm(double p)
{
    p = (fabs(p) < 1.0e-20 ? 1 : p);

    return 10.0 * log10(p);
}

double Collector::convertdBm2MilliWatt(double x)
{
    return pow(10.0, (x / 10.0));
}

