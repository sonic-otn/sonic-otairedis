#pragma once

#include "OtaiLinecard.h"
#include "VirtualOidTranslator.h"
#include "RedisClient.h"
#include "NotificationHandler.h"

#include "lib/inc/OtaiInterface.h"

#include "meta/OtaiAttributeList.h"
#include "FlexCounterManager.h"

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <memory>

namespace syncd
{
    class FlexCounterReiniter
    {
    public:
        typedef std::unordered_map<std::string, std::string> StringHash;
        typedef std::unordered_map<otai_object_id_t, otai_object_id_t> ObjectIdMap;
    public:
        FlexCounterReiniter(
            _In_ std::shared_ptr<RedisClient> client,
            _In_ std::shared_ptr<VirtualOidTranslator> translator,
            _In_ std::shared_ptr<FlexCounterManager> manager,
            _In_ const ObjectIdMap& vidToRidMap,
            _In_ const ObjectIdMap& ridToVidMap,
            _In_ const std::vector<std::string>& flexCounterKeys);

        virtual ~FlexCounterReiniter();
    private:
        void prepareFlexCounterState();
        void getInfoFromFlexCounterKey(_In_ const std::string& key, _Out_ std::string& instancdID, _Out_ otai_object_id_t& vid, _Out_ otai_object_id_t& rid);
        std::vector<swss::FieldValueTuple> redisGetAttributesFromKey(_In_ const std::string& key);
    public:
        void hardReinit();
    private:
        std::shared_ptr<RedisClient> m_client;
        std::shared_ptr<VirtualOidTranslator> m_translator;
        std::shared_ptr<FlexCounterManager> m_manager;
        ObjectIdMap m_vidToRidMap;
        ObjectIdMap m_ridToVidMap;
        std::vector<std::string> m_flexCounterKeys;
        otai_object_id_t m_linecard_rid;
        otai_object_id_t m_linecard_vid;
        std::vector<swss::FieldValueTuple> m_values;
    };

    class FlexCounterGroupReiniter
    {
    public:
        FlexCounterGroupReiniter(
            _In_ std::shared_ptr<RedisClient> client,
            _In_ std::shared_ptr<FlexCounterManager> manager,
            _In_ const std::vector<std::string>& flexCounterGroupKeys);
        virtual ~FlexCounterGroupReiniter();
    private:
        void prepareFlexCounterGroupState();
        std::string getGroupNameFromGroupKey(_In_ const std::string& key);
        std::vector<swss::FieldValueTuple> redisGetAttributesFromKey(_In_ const std::string& key);
    public:
        void hardReinit();
    private:
        std::shared_ptr<RedisClient> m_client;
        std::shared_ptr<FlexCounterManager> m_manager;
        std::vector<std::string> m_flexCounterGroupKeys;
        std::vector<swss::FieldValueTuple> m_values;
    };
}
