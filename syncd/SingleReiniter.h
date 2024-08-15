#pragma once

#include "OtaiLinecard.h"
#include "VirtualOidTranslator.h"
#include "RedisClient.h"
#include "NotificationHandler.h"

#include "meta/OtaiInterface.h"

#include "meta/OtaiAttributeList.h"
#include "FlexCounterManager.h"

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <memory>

namespace syncd
{
    class SingleReiniter
    {
    public:

        typedef std::unordered_map<std::string, std::string> StringHash;
        typedef std::unordered_map<otai_object_id_t, otai_object_id_t> ObjectIdMap;

    public:

        SingleReiniter(
            _In_ std::shared_ptr<RedisClient> client,
            _In_ std::shared_ptr<VirtualOidTranslator> translator,
            _In_ std::shared_ptr<otairedis::OtaiInterface> otai,
            _In_ std::shared_ptr<NotificationHandler> handler,
            _In_ const ObjectIdMap& vidToRidMap,
            _In_ const ObjectIdMap& ridToVidMap,
            _In_ const std::vector<std::string>& asicKeys);

        virtual ~SingleReiniter();

    public:

        std::shared_ptr<OtaiLinecard> hardReinit();

        void postRemoveActions();

        ObjectIdMap getTranslatedVid2Rid() const;

        std::shared_ptr<OtaiLinecard> getLinecard() const;

        void setBoardMode(std::string mode);

    private:

        void prepareAsicState();

        void prepareFlexCounter();

        void prepareFlexCounterGroup();

        void checkAllIds();

        void processLinecards();

        void processOids();

        void stopPreConfigLinecards();

        otai_object_id_t processSingleVid(
            _In_ otai_object_id_t vid);

        std::shared_ptr<otaimeta::OtaiAttributeList> redisGetAttributesFromAsicKey(
            _In_ const std::string& key);

        void processAttributesForOids(
            _In_ otai_object_type_t objectType,
            _In_ uint32_t attr_count,
            _In_ otai_attribute_t* attr_list);

        void processStructNonObjectIds(
            _In_ otai_object_meta_key_t& meta_key);

        void listFailedAttributes(
            _In_ otai_object_type_t objectType,
            _In_ uint32_t attrCount,
            _In_ const otai_attribute_t* attrList);

    public:

        static otai_object_type_t getObjectTypeFromAsicKey(
            _In_ const std::string& key);

        static std::string getObjectIdFromAsicKey(
            _In_ const std::string& key);

    private:

        std::shared_ptr<otairedis::OtaiInterface> m_vendorOtai;

        ObjectIdMap m_translatedV2R;
        ObjectIdMap m_translatedR2V;

        ObjectIdMap m_vidToRidMap;
        ObjectIdMap m_ridToVidMap;

        StringHash m_oids;
        StringHash m_linecards;

        std::vector<std::string> m_asicKeys;

        std::unordered_map<std::string, std::shared_ptr<otaimeta::OtaiAttributeList>> m_attributesLists;

        std::map<otai_object_type_t, std::tuple<int, double>> m_perf_create;
        std::map<otai_object_type_t, std::tuple<int, double>> m_perf_set;

        otai_object_id_t m_linecard_rid;
        otai_object_id_t m_linecard_vid;

        std::shared_ptr<OtaiLinecard> m_sw;

        std::shared_ptr<VirtualOidTranslator> m_translator;

        std::shared_ptr<RedisClient> m_client;

        std::shared_ptr<NotificationHandler> m_handler;
    };
}
