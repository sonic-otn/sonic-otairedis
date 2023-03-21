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

#include <inttypes.h>

#include "LaiGaugeCollector.h"
#include "meta/lai_serialize.h"

extern "C" {
#include "lai.h"
}

using namespace std;
using namespace syncd;

LaiGaugeCollector::LaiGaugeCollector(
            _In_ lai_object_type_t objectType,
            _In_ lai_object_id_t vid,
            _In_ lai_object_id_t rid,
            std::shared_ptr<lairedis::LaiInterface> vendorLai,
            _In_ const std::set<std::string> &strStatIds) :
            Collector(objectType, vid, rid, vendorLai)
{
    SWSS_LOG_ENTER();

    for (const string &strStatId : strStatIds)
    {
        const lai_stat_metadata_t *meta;

        lai_deserialize_stat_id(strStatId, &meta);

        lai_status_t status;
        lai_stat_value_t value;
        lai_stat_id_t statid = meta->statid;

        status = vendorLai->getStats(objectType, rid, 1, &statid, &value);
        if (status == LAI_STATUS_SUCCESS ||
            status == LAI_STATUS_UNINITIALIZED ||
            status == LAI_STATUS_OBJECT_NOT_READY)
        {
            m_entries.push_back(entry(meta, m_countersTableKeyName)); 
        }
        else
        {
            SWSS_LOG_WARN("Unsupported stat:%s oid:0x%" PRIX64 ", status:%d",
                          strStatId.c_str(), rid, status);
        }
    }

    for (auto &e : m_entries)
    {
        e.m_statvalue15min.m_interval = PM_CYCLE_15_MINS;
        e.m_statvalue24hour.m_interval = PM_CYCLE_24_HOURS;

        e.m_statvalue15min.m_expiretime = EXPIRE_TIME_2_DAYS;
        e.m_statvalue24hour.m_expiretime = EXPIRE_TIME_7_DAYS;
    }    
}

LaiGaugeCollector::~LaiGaugeCollector()
{
    SWSS_LOG_ENTER();

    /* clear all gauge data in db */

    for (auto &e : m_entries)
    {
        m_countersTable->del(e.m_key15min);
        m_countersTable->del(e.m_key24hour);

        SWSS_LOG_NOTICE("Clear gauge data, table:%s,%s", 
                        e.m_key15min.c_str(), e.m_key24hour.c_str());
    }
}

void LaiGaugeCollector::collect()
{
    SWSS_LOG_ENTER();

    lai_status_t status;

    updateTimeFlags();

    for (auto &e : m_entries)
    {
        status = m_vendorLai->getStats(m_objectType,
                                       m_rid,
                                       1,
                                       &e.m_statid,
                                       &e.m_statvalue);

        if (status != LAI_STATUS_SUCCESS)
        {
            e.m_statvalue15min.m_failurecount++;
            e.m_statvalue24hour.m_failurecount++;

            continue;
        }

        updatePeriodicValue(e, STAT_CYCLE_15_MINS);
        updatePeriodicValue(e, STAT_CYCLE_24_HOURS);
    }
}

void LaiGaugeCollector::updatePeriodicValue(entry &e, StatisticalCycle cycle)
{
    SWSS_LOG_ENTER();

    bool timeout;
    string key;
    string historyKey;

    if (cycle == STAT_CYCLE_15_MINS)
    {
        key = e.m_key15min;
        historyKey = e.m_historyKey15min;
        timeout = m_timeout15min;
    }
    else
    {
        key = e.m_key24hour;
        historyKey = e.m_historyKey24hour;
        timeout = m_timeout24hour;
    }

    AvgMinMaxValue &v = (cycle == STAT_CYCLE_15_MINS) ? e.m_statvalue15min : e.m_statvalue24hour;

    if (v.m_init || timeout)
    {
        /* save to history db */
        if (!v.m_init)
        {
            historyKey += to_string(v.m_starttime);
            m_historyTable->hset(historyKey, "starttime", to_string(v.m_starttime));
            m_historyTable->hset(historyKey, "interval", to_string(v.m_interval));

            if (v.m_validityType == VALIDITY_TYPE_INCOMPLETE &&
                v.m_failurecount == 0)
            {
                v.m_validityType = VALIDITY_TYPE_COMPLETE;
            }
            m_historyTable->hset(historyKey, "validity", validityToString(v.m_validityType));

            m_historyTable->hset(historyKey, "max", lai_serialize_stat_value(*e.m_meta, v.m_maxvalue));
            m_historyTable->hset(historyKey, "max-time", to_string(v.m_maxtime));
            m_historyTable->hset(historyKey, "min", lai_serialize_stat_value(*e.m_meta, v.m_minvalue));
            m_historyTable->hset(historyKey, "min-time", to_string(v.m_mintime));
            m_historyTable->hset(historyKey, "avg", lai_serialize_stat_value(*e.m_meta, v.m_avgvalue));
            m_historyTable->hset(historyKey, "instant", lai_serialize_stat_value(*e.m_meta, v.m_instantvalue));
            m_historyTable->expire(historyKey, v.m_expiretime);
        }
        else
        {
            v.m_init = false;
            m_countersTable->hset(key, "interval", to_string(v.m_interval));
        }

        v.m_failurecount = 0;
        
        transfer_stat(*e.m_meta, e.m_statvalue, v.m_maxvalue);
        transfer_stat(*e.m_meta, e.m_statvalue, v.m_minvalue);
        transfer_stat(*e.m_meta, e.m_statvalue, v.m_accvalue);

        /*
         * When calculate the arithmetic mean value of a power type statistics with dBm unit,
         * use miliwatt unit. After calculation, when save the avg value to redis, use dBm unit.
         */

        if (e.m_meta->statvalueunit == LAI_STAT_VALUE_UNIT_DBM &&
            e.m_meta->statvaluetype == LAI_STAT_VALUE_TYPE_DOUBLE)
        {
            v.m_accvalue.d64 = convertdBm2MilliWatt(e.m_statvalue.d64);
        }

        transfer_stat(*e.m_meta, e.m_statvalue, v.m_avgvalue);
        transfer_stat(*e.m_meta, e.m_statvalue, v.m_instantvalue);

        v.m_maxtime = m_collectTime;
        v.m_mintime = m_collectTime;

        if (cycle == STAT_CYCLE_15_MINS)
        {
            v.m_starttime = m_counter15min * PM_CYCLE_15_MINS;
        }
        else
        {
            v.m_starttime = m_counter24hour * PM_CYCLE_24_HOURS;
        }

        v.m_accnum = 1;

        m_countersTable->hset(key, "starttime", to_string(v.m_starttime));
        m_countersTable->hset(key, "max", lai_serialize_stat_value(*e.m_meta, v.m_maxvalue));
        m_countersTable->hset(key, "max-time", to_string(v.m_maxtime));
        m_countersTable->hset(key, "min", lai_serialize_stat_value(*e.m_meta, v.m_minvalue));
        m_countersTable->hset(key, "min-time", to_string(v.m_mintime));
        m_countersTable->hset(key, "instant", lai_serialize_stat_value(*e.m_meta, v.m_instantvalue));
        m_countersTable->hset(key, "avg", lai_serialize_stat_value(*e.m_meta, v.m_avgvalue));

        v.m_currentValidityType = VALIDITY_TYPE_COMPLETE;
        m_countersTable->hset(key, "current_validity", validityToString(v.m_currentValidityType));

        v.m_validityType = VALIDITY_TYPE_INCOMPLETE;
        m_countersTable->hset(key, "validity", validityToString(v.m_validityType));

        return;
    }

    if (compare_stats(m_objectType, e.m_statid, e.m_statvalue, v.m_maxvalue) > 0)
    {
        transfer_stat(*e.m_meta, e.m_statvalue, v.m_maxvalue);
        v.m_maxtime = m_collectTime;

        m_countersTable->hset(key, "max", lai_serialize_stat_value(*e.m_meta, v.m_maxvalue));
        m_countersTable->hset(key, "max-time", to_string(v.m_maxtime));
    }

    if (compare_stats(m_objectType, e.m_statid, e.m_statvalue, v.m_minvalue) < 0)
    {
        transfer_stat(*e.m_meta, e.m_statvalue, v.m_minvalue);
        v.m_mintime = m_collectTime;

        m_countersTable->hset(key, "min", lai_serialize_stat_value(*e.m_meta, v.m_minvalue));
        m_countersTable->hset(key, "min-time", to_string(v.m_mintime));
    }

    if (compare_stats(m_objectType, e.m_statid, e.m_statvalue, v.m_instantvalue))
    {
        transfer_stat(*e.m_meta, e.m_statvalue, v.m_instantvalue);

        m_countersTable->hset(key, "instant", lai_serialize_stat_value(*e.m_meta, v.m_instantvalue));
    }

    lai_stat_value_t avgvalue;

    if (e.m_meta->statvalueunit == LAI_STAT_VALUE_UNIT_DBM &&
        e.m_meta->statvaluetype == LAI_STAT_VALUE_TYPE_DOUBLE)
    {
        v.m_accvalue.d64 += convertdBm2MilliWatt(e.m_statvalue.d64);
        v.m_accnum++;
        avgvalue.d64 = convertMilliWatt2dBm(v.m_accvalue.d64 / v.m_accnum);
    }
    else
    {
        inc_stat(*e.m_meta, v.m_accvalue, e.m_statvalue);
        v.m_accnum++;
        div_stat(*e.m_meta, avgvalue, v.m_accvalue, v.m_accnum);
    }

    if (compare_stats(m_objectType, e.m_statid, avgvalue, v.m_avgvalue))
    {
        transfer_stat(*e.m_meta, avgvalue, v.m_avgvalue);
        m_countersTable->hset(key, "avg", lai_serialize_stat_value(*e.m_meta, v.m_avgvalue));
    }
}

