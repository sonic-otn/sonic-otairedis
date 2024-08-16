#pragma once

extern "C" {
#include "otaimetadata.h"
}

#include "swss/table.h"

#include <string>
#include <unordered_map>
#include <set>
#include <memory>
#include <vector>

namespace syncd
{
    class RedisClient
    {
        public:

            RedisClient(
                    _In_ std::shared_ptr<swss::DBConnector> dbAsic,
                    _In_ std::shared_ptr<swss::DBConnector> dbFlexCounter);

            virtual ~RedisClient();

        public:
            std::unordered_map<otai_object_id_t, otai_object_id_t> getVidToRidMap(
                    _In_ otai_object_id_t linecardVid) const;

            std::unordered_map<otai_object_id_t, otai_object_id_t> getRidToVidMap(
                    _In_ otai_object_id_t linecardVid) const;

            std::unordered_map<otai_object_id_t, otai_object_id_t> getVidToRidMap() const;

            std::unordered_map<otai_object_id_t, otai_object_id_t> getRidToVidMap() const;

            void removeAsicObject(
                    _In_ otai_object_id_t objectVid) const;

            void removeAsicObject(
                    _In_ const otai_object_meta_key_t& metaKey);

            void removeAsicObjects(
                    _In_ const std::vector<std::string>& keys);

            void createAsicObject(
                    _In_ const otai_object_meta_key_t& metaKey,
                    _In_ const std::vector<swss::FieldValueTuple>& attrs);

            void createAsicObjects(
                    _In_ const std::unordered_map<std::string, std::vector<swss::FieldValueTuple>>& multiHash);

            void setVidAndRidMap(
                    _In_ const std::unordered_map<otai_object_id_t, otai_object_id_t>& map);

            std::vector<std::string> getAsicStateKeys() const;

            std::vector<std::string> getFlexCounterKeys() const;

            std::vector<std::string> getFlexCounterGroupKeys() const;

            std::unordered_map<std::string, std::string> getAttributesFromFlexCounterKey(_In_ const std::string& key) const;

            std::unordered_map<std::string, std::string> getAttributesFromFlexCounterGroupKey(_In_ const std::string& key) const;

            std::unordered_map<std::string, std::string> getAttributesFromAsicKey(
                    _In_ const std::string& key) const;

            void removeVidAndRid(
                    _In_ otai_object_id_t vid,
                    _In_ otai_object_id_t rid);

            void insertVidAndRid(
                    _In_ otai_object_id_t vid,
                    _In_ otai_object_id_t rid);

            otai_object_id_t getVidForRid(
                    _In_ otai_object_id_t rid);

            otai_object_id_t getRidForVid(
                    _In_ otai_object_id_t vid);

            void setAsicObject(
                    _In_ const otai_object_meta_key_t& metaKey,
                    _In_ const std::string& attr,
                    _In_ const std::string& value);

        private:
            std::unordered_map<otai_object_id_t, otai_object_id_t> getObjectMap(
                    _In_ const std::string& key) const;

        private:

            std::shared_ptr<swss::DBConnector> m_dbAsic;
            std::shared_ptr<swss::DBConnector> m_dbFlexcounter;

    };
}
