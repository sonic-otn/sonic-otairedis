#pragma once

#include "lib/inc/OtaiInterface.h"

#include <memory>
#include <set>
#include <unordered_map>

namespace syncd
{
    class OtaiDiscovery
    {
        public:

            typedef std::unordered_map<otai_object_id_t, std::unordered_map<otai_attr_id_t, otai_object_id_t>> DefaultOidMap;

        public:

            OtaiDiscovery(
                    _In_ std::shared_ptr<otairedis::OtaiInterface> otai);

            virtual ~OtaiDiscovery();

        public:

            std::set<otai_object_id_t> discover(
                    _In_ otai_object_id_t rid);

            const DefaultOidMap& getDefaultOidMap() const;

        private:

            /**
             * @brief Discover objects on the linecard.
             *
             * Method will query recursively all OID attributes (oid and list) on
             * the given object.
             *
             * This method should be called only once inside constructor right
             * after linecard has been created to obtain actual ASIC view.
             *
             * @param rid Object to discover other objects.
             * @param processed Set of already processed objects. This set will be
             * updated every time new object ID is discovered.
             */
            void discover(
                    _In_ otai_object_id_t rid,
                    _Inout_ std::set<otai_object_id_t> &processed);

            void setApiLogLevel(
                    _In_ otai_log_level_t logLevel);

        private:

            std::shared_ptr<otairedis::OtaiInterface> m_otai;

            DefaultOidMap m_defaultOidMap;
    };
}
