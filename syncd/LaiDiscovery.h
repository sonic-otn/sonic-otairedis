#pragma once

#include "lib/inc/LaiInterface.h"

#include <memory>
#include <set>
#include <unordered_map>

namespace syncd
{
    class LaiDiscovery
    {
        public:

            typedef std::unordered_map<lai_object_id_t, std::unordered_map<lai_attr_id_t, lai_object_id_t>> DefaultOidMap;

        public:

            LaiDiscovery(
                    _In_ std::shared_ptr<lairedis::LaiInterface> lai);

            virtual ~LaiDiscovery();

        public:

            std::set<lai_object_id_t> discover(
                    _In_ lai_object_id_t rid);

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
                    _In_ lai_object_id_t rid,
                    _Inout_ std::set<lai_object_id_t> &processed);

            void setApiLogLevel(
                    _In_ lai_log_level_t logLevel);

        private:

            std::shared_ptr<lairedis::LaiInterface> m_lai;

            DefaultOidMap m_defaultOidMap;
    };
}
