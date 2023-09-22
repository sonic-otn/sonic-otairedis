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

#include "OtaiStatCollector.h"
#include "meta/otai_serialize.h"

extern "C" {
#include "otai.h"
}

using namespace std;
using namespace syncd;

OtaiStatCollector::OtaiStatCollector(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t vid,
        _In_ otai_object_id_t rid,
        std::shared_ptr<otairedis::OtaiInterface> vendorOtai,
        _In_ const std::set<std::string> &strStatIds) :
        Collector(objectType, vid, rid, vendorOtai)
{
    SWSS_LOG_ENTER();

    for (const string &strStatId : strStatIds)
    {
        const otai_stat_metadata_t *meta;

        otai_deserialize_stat_id(strStatId, &meta);

        otai_status_t status;
        otai_stat_value_t value;
        otai_stat_id_t statid = meta->statid;

        status = vendorOtai->getStats(objectType, rid, 1, &statid, &value);
        if (status == OTAI_STATUS_SUCCESS ||
            status == OTAI_STATUS_UNINITIALIZED ||
            status == OTAI_STATUS_OBJECT_NOT_READY)
        {   
            m_entries.push_back(entry(meta)); 
        }   
        else
        {   
            SWSS_LOG_WARN("Unsupported stat:%s oid:0x%" PRIX64 ", status:%d",
                          strStatId.c_str(), rid, status);
        }
    }

    for (auto &e : m_entries)
    {   
        e.m_accvalue15min.m_interval = PM_CYCLE_15_MINS;
        e.m_accvalue24hour.m_interval = PM_CYCLE_24_HOURS; 

        e.m_accvalue15min.m_expiretime = EXPIRE_TIME_2_DAYS;
        e.m_accvalue24hour.m_expiretime = EXPIRE_TIME_7_DAYS;
    }

    m_keyCur = m_countersTableKeyName + ":current";
    m_key15min = m_countersTableKeyName + ":15_pm_current";
    m_key24hour = m_countersTableKeyName + ":24_pm_current";

    m_historyKey15min = m_historyTableKeyName + ":15_pm_history_";
    m_historyKey24hour = m_historyTableKeyName + ":24_pm_history_";
    
}

OtaiStatCollector::~OtaiStatCollector()
{
    SWSS_LOG_ENTER();

    /* clear all stat data in db */

    m_countersTable->del(m_keyCur);
    m_countersTable->del(m_key15min);
    m_countersTable->del(m_key24hour);

    SWSS_LOG_NOTICE("Clear counter data, table:%s,%s,%s",
                    m_keyCur.c_str(), m_key15min.c_str(), m_key24hour.c_str());
}

void OtaiStatCollector::collect()
{
    SWSS_LOG_ENTER();

    otai_status_t status;

    updateTimeFlags();

    for (auto &e : m_entries)
    {
        status = m_vendorOtai->getStats(m_objectType,
                                       m_rid,
                                       1,
                                       &e.m_statid,
                                       &e.m_statvalue);

        if (status != OTAI_STATUS_SUCCESS)
        {
            e.m_accvalue15min.m_failurecount++;
            e.m_accvalue24hour.m_failurecount++;

            continue;
        }

        updateCurrentValue(e);
        updatePeriodicValue(e, STAT_CYCLE_15_MINS);
        updatePeriodicValue(e, STAT_CYCLE_24_HOURS);

    }
}

void OtaiStatCollector::updateCurrentValue(entry &e)
{
    SWSS_LOG_ENTER();

    bool saveToRedis = false;

    AccumulativeValue &v = e.m_accvalue;

    if (v.m_init)
    {
        transfer_stat(*e.m_meta, e.m_statvalue, v.m_stataccvalue); 
        saveToRedis = true;

        v.m_init = false;
    }
    else
    {
        /*
         * In terms of counter type statistics, OTAI library will clear its value after
         * being read by syncd, so we need to accumulate the result in each collection cycle.
         */

        inc_stat(*e.m_meta, v.m_stataccvalue, e.m_statvalue);
        if (compare_stats(m_objectType, e.m_statid, v.m_stataccvalue, v.m_statvaluedb))
        {
            saveToRedis = true;
        } 
    }

    if (saveToRedis)
    {
        m_countersTable->hset(m_keyCur, otai_serialize_stat_id_kebab_case(*e.m_meta),
                              otai_serialize_stat_value(*e.m_meta, v.m_stataccvalue));
        transfer_stat(*e.m_meta, v.m_stataccvalue, v.m_statvaluedb);
    }
}

void OtaiStatCollector::updatePeriodicValue(entry &e, StatisticalCycle cycle)
{
    SWSS_LOG_ENTER();

    std::string key;
    std::string historyKey;
    bool timeout;

    if (cycle == STAT_CYCLE_15_MINS)
    {
        key = m_key15min; 
        historyKey = m_historyKey15min;
        timeout = m_timeout15min;
    }
    else
    {
        key = m_key24hour;
        historyKey = m_historyKey24hour;
        timeout = m_timeout24hour;
    }

    AccumulativeValue &accvalue = (cycle == STAT_CYCLE_15_MINS) ? e.m_accvalue15min : e.m_accvalue24hour;

    if (accvalue.m_init || timeout)
    {
        /* save to history db */
        if (!accvalue.m_init)
        {
            historyKey += to_string(accvalue.m_starttime);
            m_historyTable->hset(historyKey, "starttime", to_string(accvalue.m_starttime));
            m_historyTable->hset(historyKey, "interval", to_string(accvalue.m_interval));

            if (accvalue.m_validityType == VALIDITY_TYPE_INCOMPLETE &&
                accvalue.m_failurecount == 0)
            {
                accvalue.m_validityType = VALIDITY_TYPE_COMPLETE;
            }
            m_historyTable->hset(historyKey, "validity", validityToString(accvalue.m_validityType));
            m_historyTable->hset(historyKey, otai_serialize_stat_id_kebab_case(*e.m_meta),
                                 otai_serialize_stat_value(*e.m_meta, accvalue.m_stataccvalue));
            m_historyTable->expire(historyKey, accvalue.m_expiretime);
        }
        else
        {
            m_countersTable->hset(key,  "interval", to_string(accvalue.m_interval));
            accvalue.m_init = false;
        }

        accvalue.m_failurecount = 0;

        if (cycle == STAT_CYCLE_15_MINS)
        {
            accvalue.m_starttime = m_counter15min * PM_CYCLE_15_MINS;
        }
        else
        {
            accvalue.m_starttime = m_counter24hour * PM_CYCLE_24_HOURS;
        }

        m_countersTable->hset(key, "starttime", to_string(accvalue.m_starttime));

        transfer_stat(*e.m_meta, e.m_statvalue, accvalue.m_stataccvalue);

        m_countersTable->hset(key, otai_serialize_stat_id_kebab_case(*e.m_meta),
                              otai_serialize_stat_value(*e.m_meta, accvalue.m_stataccvalue));

        transfer_stat(*e.m_meta, accvalue.m_stataccvalue, accvalue.m_statvaluedb); 

        accvalue.m_validityType = VALIDITY_TYPE_INCOMPLETE;
        m_countersTable->hset(key, "validity", validityToString(accvalue.m_validityType));

        return;
    }

    inc_stat(*e.m_meta, accvalue.m_stataccvalue, e.m_statvalue);

    if (compare_stats(m_objectType, e.m_statid, accvalue.m_stataccvalue, accvalue.m_statvaluedb))
    {
        m_countersTable->hset(key, otai_serialize_stat_id_kebab_case(*e.m_meta),
                              otai_serialize_stat_value(*e.m_meta, accvalue.m_stataccvalue));

        transfer_stat(*e.m_meta, accvalue.m_stataccvalue, accvalue.m_statvaluedb);
    }
}
