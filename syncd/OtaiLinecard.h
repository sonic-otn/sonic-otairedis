#pragma once

extern "C" {
#include "otai.h"
}

#include "meta/OtaiInterface.h"
#include "VirtualOidTranslator.h"
#include "RedisClient.h"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <map>
#include <memory>

namespace syncd
{
    class OtaiLinecard
    {
        private:

            OtaiLinecard(const OtaiLinecard&);
            OtaiLinecard& operator=(const OtaiLinecard&);

        public:

            OtaiLinecard(
                    _In_ otai_object_id_t linecard_vid,
                    _In_ otai_object_id_t linecard_rid,
                    _In_ std::shared_ptr<RedisClient> client,
                    _In_ std::shared_ptr<VirtualOidTranslator> translator,
                    _In_ std::shared_ptr<otairedis::OtaiInterface> vendorOtai);

            ~OtaiLinecard() = default;

        public:

            std::unordered_map<otai_object_id_t, otai_object_id_t> getVidToRidMap() const;

            std::unordered_map<otai_object_id_t, otai_object_id_t> getRidToVidMap() const;

            otai_object_id_t getRid() const;

            otai_object_id_t getVid() const;

        private:
            /**
             * @brief Linecard virtual ID assigned by syncd.
             */
            otai_object_id_t m_linecard_vid;

            /**
             * @brief Linecard real ID assigned by OTAI SDK.
             */
            otai_object_id_t m_linecard_rid;

            std::shared_ptr<RedisClient> m_client;

            std::shared_ptr<VirtualOidTranslator> m_translator;

            std::shared_ptr<otairedis::OtaiInterface> m_vendorOtai;
    };
}
