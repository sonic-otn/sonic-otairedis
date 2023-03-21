#pragma once

#include "LaiInterface.h"
#include "LaiLinecard.h"
#include "VirtualOidTranslator.h"
#include "RedisClient.h"
#include "NotificationHandler.h"
#include "FlexCounterManager.h"

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <memory>

namespace syncd
{
    class HardReiniter
    {
        public:

            typedef std::unordered_map<std::string, std::string> StringHash;
            typedef std::unordered_map<lai_object_id_t, lai_object_id_t> ObjectIdMap;

        public:

            HardReiniter(
                    _In_ std::shared_ptr<RedisClient> client,
                    _In_ std::shared_ptr<VirtualOidTranslator> translator,
                    _In_ std::shared_ptr<lairedis::LaiInterface> lai,
                    _In_ std::shared_ptr<NotificationHandler> handler,
                    _In_ std::shared_ptr<FlexCounterManager> manager);

            virtual ~HardReiniter();

        public:

            std::map<lai_object_id_t, std::shared_ptr<syncd::LaiLinecard>> hardReinit();

        private:

            void readAsicState();

            void redisSetVidAndRidMap(
                    _In_ const std::unordered_map<lai_object_id_t, lai_object_id_t> &map);

        private:

            ObjectIdMap m_vidToRidMap;
            ObjectIdMap m_ridToVidMap;

            std::map<lai_object_id_t, ObjectIdMap> m_linecardVidToRid;
            std::map<lai_object_id_t, ObjectIdMap> m_linecardRidToVid;

            std::map<lai_object_id_t, std::vector<std::string>> m_linecardMap;

            std::shared_ptr<lairedis::LaiInterface> m_vendorLai;

            std::shared_ptr<VirtualOidTranslator> m_translator;

            std::shared_ptr<RedisClient> m_client;

            std::shared_ptr<NotificationHandler> m_handler;

            std::shared_ptr<FlexCounterManager> m_manager;
    };
}
