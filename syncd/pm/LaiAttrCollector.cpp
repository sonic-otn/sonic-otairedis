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

#include "LaiAttrCollector.h"

extern "C" {
#include "lai.h"
}

using namespace std;
using namespace syncd;

LaiAttrCollector::LaiAttrCollector(
            _In_ lai_object_type_t objectType,
            _In_ lai_object_id_t vid,
            _In_ lai_object_id_t rid,
            std::shared_ptr<lairedis::LaiInterface> vendorLai,
            _In_ const std::set<std::string> &strAttrIds) :
            Collector(objectType, vid, rid, vendorLai)
{
    SWSS_LOG_ENTER();

    for (const string &strAttrId : strAttrIds)
    {
        const lai_attr_metadata_t *meta;

        lai_deserialize_attr_id(strAttrId, &meta);

        if (LAI_HAS_FLAG_CREATE_ONLY(meta->flags) ||
            LAI_HAS_FLAG_SET_ONLY(meta->flags))
        {
            continue;
        }

        lai_attribute_t attr;
        memset(&attr, 0, sizeof(attr));
        attr.id = meta->attrid;
        newLaiAttr(attr, meta);
 
        lai_status_t status = vendorLai->get(objectType, rid, 1, &attr);
        if (status == LAI_STATUS_SUCCESS ||
            status == LAI_STATUS_UNINITIALIZED ||
            status == LAI_STATUS_OBJECT_NOT_READY)
        {
            m_entries.push_back(entry(meta));
        }
        else
        {
            SWSS_LOG_WARN("Unsupported attr:%s oid:0x%" PRIX64 ", status:%d",
                          strAttrId.c_str(), rid, status);
        }

        freeLaiAttr(attr, meta);
    }

    for (auto &e : m_entries)
    {
       newLaiAttr(e.m_attr, e.m_meta); 
       newLaiAttr(e.m_attrdb, e.m_meta); 
    }
}

LaiAttrCollector::~LaiAttrCollector()
{
    SWSS_LOG_ENTER();

    /* clear state data in db */
    for (auto &e : m_entries)
    {
        string field = lai_serialize_attr_id_kebab_case(*e.m_meta);
        m_stateTable->hdel(m_stateTableKeyName, field);

        SWSS_LOG_ERROR("Clear state data, table:%s, field:%s",
                       m_stateTableKeyName.c_str(), field.c_str());
    }

    for (auto &e : m_entries)
    {
        freeLaiAttr(e.m_attr, e.m_meta);
        freeLaiAttr(e.m_attrdb, e.m_meta);
    }
}

void LaiAttrCollector::collect()
{
    SWSS_LOG_ENTER();

    lai_status_t status;

    for (auto &e : m_entries)
    {
        status = m_vendorLai->get(m_objectType, m_rid, 1, &e.m_attr);

        if (status == LAI_STATUS_UNINITIALIZED ||
            status == LAI_STATUS_OBJECT_NOT_READY)
        {
            continue;
        }
        else if (status != LAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to get attr, oid:0x%" PRIx64 ", attrid:%s, status:%d",
                           m_rid, lai_serialize_attr_id(*e.m_meta).c_str(), status);
            continue;
        }

        updateCurrentValue(e);

    }
}

void LaiAttrCollector::updateCurrentValue(entry &e)
{
    SWSS_LOG_ENTER();

    bool saveToRedis = false;

    if (e.m_init == true)
    {
        saveToRedis = true;
        e.m_init = false;
    }
    else if (compare_attribute(m_objectType, e.m_attrdb, e.m_attr))
    {
        saveToRedis = true;
    }

    if (saveToRedis)
    {
        m_stateTable->hset(m_stateTableKeyName, lai_serialize_attr_id_kebab_case(*e.m_meta),
                           lai_serialize_attr_value(*e.m_meta, e.m_attr, false, true));

        transfer_attributes(m_objectType, 1, &e.m_attr, &e.m_attrdb, false);
    }
}

void LaiAttrCollector::newLaiAttr(
    _Inout_ lai_attribute_t &attr,
    _In_ const lai_attr_metadata_t *meta)
{
#define LIST_DEFAULT_LENGTH 512

    SWSS_LOG_ENTER();

    if (meta == NULL)
    {
        return;
    }

    switch (meta->attrvaluetype)
    {
    case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:
        attr.value.objlist.count = LIST_DEFAULT_LENGTH;
        attr.value.objlist.list = new lai_object_id_t[LIST_DEFAULT_LENGTH];
        break;
    case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
        attr.value.u8list.count = LIST_DEFAULT_LENGTH;
        attr.value.u8list.list = new uint8_t[LIST_DEFAULT_LENGTH];
        break;
    case LAI_ATTR_VALUE_TYPE_INT8_LIST:
        attr.value.s8list.count = LIST_DEFAULT_LENGTH;
        attr.value.s8list.list = new int8_t[LIST_DEFAULT_LENGTH];
        break;
    case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
        attr.value.u16list.count = LIST_DEFAULT_LENGTH;
        attr.value.u16list.list = new uint16_t[LIST_DEFAULT_LENGTH];
        break;
    case LAI_ATTR_VALUE_TYPE_INT16_LIST:
        attr.value.s16list.count = LIST_DEFAULT_LENGTH;
        attr.value.s16list.list = new int16_t[LIST_DEFAULT_LENGTH];
        break;
    case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
        attr.value.u32list.count = LIST_DEFAULT_LENGTH;
        attr.value.u32list.list = new uint32_t[LIST_DEFAULT_LENGTH];
        break;
    case LAI_ATTR_VALUE_TYPE_INT32_LIST:
        attr.value.s32list.count = LIST_DEFAULT_LENGTH;
        attr.value.s32list.list = new int32_t[LIST_DEFAULT_LENGTH];
        break;
    default:
        break;
    }
}

void LaiAttrCollector::freeLaiAttr(
    _Inout_ lai_attribute_t &attr,
    _In_ const lai_attr_metadata_t *meta)
{
    SWSS_LOG_ENTER();

    if (meta == NULL)
    {
        return;
    }

    switch (meta->attrvaluetype)
    {
    case LAI_ATTR_VALUE_TYPE_OBJECT_LIST:
        delete[] attr.value.objlist.list;
        break;
    case LAI_ATTR_VALUE_TYPE_UINT8_LIST:
        delete[] attr.value.u8list.list;
        break;
    case LAI_ATTR_VALUE_TYPE_INT8_LIST:
        delete[] attr.value.s8list.list;
        break;
    case LAI_ATTR_VALUE_TYPE_UINT16_LIST:
        delete[] attr.value.u16list.list;
        break;
    case LAI_ATTR_VALUE_TYPE_INT16_LIST:
        delete[] attr.value.s16list.list;
        break;
    case LAI_ATTR_VALUE_TYPE_UINT32_LIST:
        delete[] attr.value.u32list.list;
        break;
    case LAI_ATTR_VALUE_TYPE_INT32_LIST:
        delete[] attr.value.s32list.list;
        break;
    default:
        break;
    }
}
