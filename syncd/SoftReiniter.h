#pragma once

#include "LaiInterface.h"
#include "LaiLinecard.h"
#include "VirtualOidTranslator.h"
#include "RedisClient.h"
#include "NotificationHandler.h"
#include "FlexCounterManager.h"
#include "meta/LaiAttributeList.h"

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
            typedef unordered_map<lai_object_id_t, lai_object_id_t> ObjectIdMap;

        public:
            SoftReiniter(
                _In_ shared_ptr<RedisClient> client,
                _In_ std::shared_ptr<VirtualOidTranslator> translator,
                _In_ std::shared_ptr<lairedis::LaiInterface> lai,
                _In_ std::shared_ptr<FlexCounterManager> manager
            );
            virtual ~SoftReiniter();
            void readAsicState();
            void prepareAsicState(vector<string> &asicKeys);
            void softReinit();
            lai_object_type_t getObjectTypeFromAsicKey(_In_ const string& key);
            string getObjectIdFromAsicKey(_In_ const string& key);
            shared_ptr<laimeta::LaiAttributeList> redisGetAttributesFromAsicKey(_In_ const string& key);
            void processLinecards();
            void stopPreConfigLinecards();
            void processOids();
            lai_object_id_t processSingleVid(_In_ lai_object_id_t vid);
            void setBoardMode(std::string mode);
        private:
            std::shared_ptr<lairedis::LaiInterface> m_vendorLai;

            ObjectIdMap m_translatedV2R;
            ObjectIdMap m_translatedR2V;

            ObjectIdMap m_vidToRidMap;
            ObjectIdMap m_ridToVidMap;
            StringHash m_oids;
            StringHash m_linecards;
            unordered_map<string, shared_ptr<laimeta::LaiAttributeList>> m_attributesLists;

            lai_object_id_t m_linecard_rid;
            lai_object_id_t m_linecard_vid;

            map<lai_object_id_t, ObjectIdMap> m_linecardVidToRid;
            map<lai_object_id_t, ObjectIdMap> m_linecardRidToVid;
            map<lai_object_id_t, vector<string>> m_linecardMap;
            std::shared_ptr<VirtualOidTranslator> m_translator;
            shared_ptr<RedisClient> m_client;
            std::shared_ptr<FlexCounterManager> m_manager;
    };
}
