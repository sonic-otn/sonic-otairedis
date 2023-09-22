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

#include "OtaiAttrCollector.h"

extern "C" {
#include "otai.h"
}

using namespace std;
using namespace syncd;

OtaiAttrCollector::OtaiAttrCollector(
            _In_ otai_object_type_t objectType,
            _In_ otai_object_id_t vid,
            _In_ otai_object_id_t rid,
            std::shared_ptr<otairedis::OtaiInterface> vendorOtai,
            _In_ const std::set<std::string> &strAttrIds) :
            Collector(objectType, vid, rid, vendorOtai)
{
    SWSS_LOG_ENTER();

    for (const string &strAttrId : strAttrIds)
    {
        const otai_attr_metadata_t *meta;

        otai_deserialize_attr_id(strAttrId, &meta);

        if (OTAI_HAS_FLAG_CREATE_ONLY(meta->flags) ||
            OTAI_HAS_FLAG_SET_ONLY(meta->flags))
        {
            continue;
        }

        otai_attribute_t attr;
        memset(&attr, 0, sizeof(attr));
        attr.id = meta->attrid;
        newOtaiAttr(attr, meta);
 
        otai_status_t status = vendorOtai->get(objectType, rid, 1, &attr);
        if (status == OTAI_STATUS_SUCCESS ||
            status == OTAI_STATUS_UNINITIALIZED ||
            status == OTAI_STATUS_OBJECT_NOT_READY)
        {
            m_entries.push_back(entry(meta));
        }
        else
        {
            SWSS_LOG_WARN("Unsupported attr:%s oid:0x%" PRIX64 ", status:%d",
                          strAttrId.c_str(), rid, status);
        }

        freeOtaiAttr(attr, meta);
    }

    for (auto &e : m_entries)
    {
       newOtaiAttr(e.m_attr, e.m_meta); 
       newOtaiAttr(e.m_attrdb, e.m_meta); 
    }
}

OtaiAttrCollector::~OtaiAttrCollector()
{
    SWSS_LOG_ENTER();

    /* clear state data in db */
    for (auto &e : m_entries)
    {
        string field = otai_serialize_attr_id_kebab_case(*e.m_meta);
        m_stateTable->hdel(m_stateTableKeyName, field);

        SWSS_LOG_NOTICE("Clear state data, table:%s, field:%s",
                       m_stateTableKeyName.c_str(), field.c_str());
    }

    for (auto &e : m_entries)
    {
        freeOtaiAttr(e.m_attr, e.m_meta);
        freeOtaiAttr(e.m_attrdb, e.m_meta);
    }
}

void OtaiAttrCollector::collect()
{
    SWSS_LOG_ENTER();

    otai_status_t status;

    for (auto &e : m_entries)
    {
        status = m_vendorOtai->get(m_objectType, m_rid, 1, &e.m_attr);

        if (status == OTAI_STATUS_UNINITIALIZED ||
            status == OTAI_STATUS_OBJECT_NOT_READY)
        {
            continue;
        }
        else if (status != OTAI_STATUS_SUCCESS)
        {
            SWSS_LOG_ERROR("Failed to get attr, oid:0x%" PRIx64 ", attrid:%s, status:%d",
                           m_rid, otai_serialize_attr_id(*e.m_meta).c_str(), status);
            continue;
        }

        updateCurrentValue(e);

    }
}

void OtaiAttrCollector::updateCurrentValue(entry &e)
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
        m_stateTable->hset(m_stateTableKeyName, otai_serialize_attr_id_kebab_case(*e.m_meta),
                           otai_serialize_attr_value(*e.m_meta, e.m_attr, false, true));

        transfer_attributes(m_objectType, 1, &e.m_attr, &e.m_attrdb, false);
    }
}

void OtaiAttrCollector::newOtaiAttr(
    _Inout_ otai_attribute_t &attr,
    _In_ const otai_attr_metadata_t *meta)
{
#define LIST_DEFAULT_LENGTH 512

    SWSS_LOG_ENTER();

    if (meta == NULL)
    {
        return;
    }

    switch (meta->attrvaluetype)
    {
    case OTAI_ATTR_VALUE_TYPE_OBJECT_LIST:
        attr.value.objlist.count = LIST_DEFAULT_LENGTH;
        attr.value.objlist.list = new otai_object_id_t[LIST_DEFAULT_LENGTH];
        break;
    case OTAI_ATTR_VALUE_TYPE_UINT8_LIST:
        attr.value.u8list.count = LIST_DEFAULT_LENGTH;
        attr.value.u8list.list = new uint8_t[LIST_DEFAULT_LENGTH];
        break;
    case OTAI_ATTR_VALUE_TYPE_INT8_LIST:
        attr.value.s8list.count = LIST_DEFAULT_LENGTH;
        attr.value.s8list.list = new int8_t[LIST_DEFAULT_LENGTH];
        break;
    case OTAI_ATTR_VALUE_TYPE_UINT16_LIST:
        attr.value.u16list.count = LIST_DEFAULT_LENGTH;
        attr.value.u16list.list = new uint16_t[LIST_DEFAULT_LENGTH];
        break;
    case OTAI_ATTR_VALUE_TYPE_INT16_LIST:
        attr.value.s16list.count = LIST_DEFAULT_LENGTH;
        attr.value.s16list.list = new int16_t[LIST_DEFAULT_LENGTH];
        break;
    case OTAI_ATTR_VALUE_TYPE_UINT32_LIST:
        attr.value.u32list.count = LIST_DEFAULT_LENGTH;
        attr.value.u32list.list = new uint32_t[LIST_DEFAULT_LENGTH];
        break;
    case OTAI_ATTR_VALUE_TYPE_INT32_LIST:
        attr.value.s32list.count = LIST_DEFAULT_LENGTH;
        attr.value.s32list.list = new int32_t[LIST_DEFAULT_LENGTH];
        break;
    default:
        break;
    }
}

void OtaiAttrCollector::freeOtaiAttr(
    _Inout_ otai_attribute_t &attr,
    _In_ const otai_attr_metadata_t *meta)
{
    SWSS_LOG_ENTER();

    if (meta == NULL)
    {
        return;
    }

    switch (meta->attrvaluetype)
    {
    case OTAI_ATTR_VALUE_TYPE_OBJECT_LIST:
        delete[] attr.value.objlist.list;
        break;
    case OTAI_ATTR_VALUE_TYPE_UINT8_LIST:
        delete[] attr.value.u8list.list;
        break;
    case OTAI_ATTR_VALUE_TYPE_INT8_LIST:
        delete[] attr.value.s8list.list;
        break;
    case OTAI_ATTR_VALUE_TYPE_UINT16_LIST:
        delete[] attr.value.u16list.list;
        break;
    case OTAI_ATTR_VALUE_TYPE_INT16_LIST:
        delete[] attr.value.s16list.list;
        break;
    case OTAI_ATTR_VALUE_TYPE_UINT32_LIST:
        delete[] attr.value.u32list.list;
        break;
    case OTAI_ATTR_VALUE_TYPE_INT32_LIST:
        delete[] attr.value.s32list.list;
        break;
    default:
        break;
    }
}
