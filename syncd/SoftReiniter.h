#pragma once

#include "meta/OtaiInterface.h"
#include "OtaiLinecard.h"
#include "VirtualOidTranslator.h"
#include "RedisClient.h"
#include "NotificationHandler.h"
#include "FlexCounterManager.h"
#include "meta/OtaiAttributeList.h"

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <memory>

using namespace std;

namespace syncd
{
    class SoftReiniter
    {
        public:
            typedef unordered_map<string, string> StringHash;
            typedef unordered_map<otai_object_id_t, otai_object_id_t> ObjectIdMap;

        public:
            SoftReiniter(
                _In_ shared_ptr<RedisClient> client,
                _In_ std::shared_ptr<VirtualOidTranslator> translator,
                _In_ std::shared_ptr<otairedis::OtaiInterface> otai,
                _In_ std::shared_ptr<FlexCounterManager> manager
            );
            virtual ~SoftReiniter();
            void readAsicState();
            void prepareAsicState(vector<string> &asicKeys);
            void softReinit();
            otai_object_type_t getObjectTypeFromAsicKey(_In_ const string& key);
            string getObjectIdFromAsicKey(_In_ const string& key);
            shared_ptr<otaimeta::OtaiAttributeList> redisGetAttributesFromAsicKey(_In_ const string& key);
            void processLinecards();
            void stopPreConfigLinecards();
            void processOids();
            otai_object_id_t processSingleVid(_In_ otai_object_id_t vid);
            void setBoardMode(std::string mode);
        private:
            std::shared_ptr<otairedis::OtaiInterface> m_vendorOtai;

            ObjectIdMap m_translatedV2R;
            ObjectIdMap m_translatedR2V;

            ObjectIdMap m_vidToRidMap;
            ObjectIdMap m_ridToVidMap;
            StringHash m_oids;
            StringHash m_linecards;
            unordered_map<string, shared_ptr<otaimeta::OtaiAttributeList>> m_attributesLists;

            otai_object_id_t m_linecard_rid;
            otai_object_id_t m_linecard_vid;

            map<otai_object_id_t, ObjectIdMap> m_linecardVidToRid;
            map<otai_object_id_t, ObjectIdMap> m_linecardRidToVid;
            map<otai_object_id_t, vector<string>> m_linecardMap;
            std::shared_ptr<VirtualOidTranslator> m_translator;
            shared_ptr<RedisClient> m_client;
            std::shared_ptr<FlexCounterManager> m_manager;
    };
}
