#pragma once

extern "C"{
#include "laimetadata.h"
}

#include "VirtualObjectIdManager.h"
#include "RedisClient.h"

#include "LaiInterface.h"

#include <mutex>
#include <unordered_map>
#include <memory>

// TODO can be child class (redis translator etc)

namespace syncd
{
    class VirtualOidTranslator
    {
        public:

            VirtualOidTranslator(
                    _In_ std::shared_ptr<RedisClient> client,
                    _In_ std::shared_ptr<lairedis::VirtualObjectIdManager> virtualObjectIdManager,
                    _In_ std::shared_ptr<lairedis::LaiInterface> vendorLai);


            virtual ~VirtualOidTranslator() = default;

        public:

            /*
             * This method will create VID for actual RID retrieved from device
             * when doing GET api and snooping while in init view mode.
             *
             * This function should not be used to create VID for LINECARD object
             * type.
             */
            lai_object_id_t translateRidToVid(
                    _In_ lai_object_id_t rid,
                    _In_ lai_object_id_t linecardVid,
                    _In_ bool translateRemoved = false);

            /*
             * This method will try get VID for given RID.
             * Returns true if input RID is null object and out VID is null object.
             * Returns true if able to find RID and out VID object.
             * Returns false if not able to find RID and out VID is null object.
             */
            bool tryTranslateRidToVid(
                    _In_ lai_object_id_t rid,
                    _Out_ lai_object_id_t &vid);

            void translateRidToVid(
                    _Inout_ lai_object_list_t& objectList,
                    _In_ lai_object_id_t linecardVid,
                    _In_ bool translateRemoved = false);

            /*
             * This method is required to translate RID to VIDs when we are doing
             * snoop for new ID's in init view mode, on in apply view mode when we
             * are executing GET api, and new object RIDs were spotted the we will
             * create new VIDs for those objects and we will put them to redis db.
             */
            void translateRidToVid(
                    _In_ lai_object_type_t objectType,
                    _In_ lai_object_id_t linecardVid,
                    _In_ uint32_t attrCount,
                    _Inout_ lai_attribute_t *attrList,
                    _In_ bool translateRemoved = false);

            /**
             * @brief Check if RID exists on the ASIC DB.
             *
             * @param rid Real object id to check.
             *
             * @return True if exists or SAI_NULL_OBJECT_ID, otherwise false.
             */
            bool checkRidExists(
                    _In_ lai_object_id_t rid,
                    _In_ bool checkRemoved = false);

            lai_object_id_t translateVidToRid(
                    _In_ lai_object_id_t vid);

            void translateVidToRid(
                    _Inout_ lai_object_list_t &element);

            void translateVidToRid(
                    _In_ lai_object_type_t objectType,
                    _In_ uint32_t attrCount,
                    _Inout_ lai_attribute_t *attrList);

            bool tryTranslateVidToRid(
                    _In_ lai_object_id_t vid,
                    _Out_ lai_object_id_t& rid);

            void translateVidToRid(
                    _Inout_ lai_object_meta_key_t &metaKey);

            bool tryTranslateVidToRid(
                    _Inout_ lai_object_meta_key_t &metaKey);

            void eraseRidAndVid(
                    _In_ lai_object_id_t rid,
                    _In_ lai_object_id_t vid);

            void insertRidAndVid(
                    _In_ lai_object_id_t rid,
                    _In_ lai_object_id_t vid);

            void clearLocalCache();

        private:

            std::shared_ptr<lairedis::VirtualObjectIdManager> m_virtualObjectIdManager;

            std::shared_ptr<lairedis::LaiInterface> m_vendorLai;

            std::mutex m_mutex;

            // those hashes keep mapping from all linecards

            std::unordered_map<lai_object_id_t, lai_object_id_t> m_rid2vid;
            std::unordered_map<lai_object_id_t, lai_object_id_t> m_vid2rid;
            std::unordered_map<lai_object_id_t, lai_object_id_t> m_removedRid2vid;

            std::shared_ptr<RedisClient> m_client;
    };
}
