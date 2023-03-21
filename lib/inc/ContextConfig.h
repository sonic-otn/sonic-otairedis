#pragma once

#include "LinecardConfigContainer.h"

namespace lairedis
{
    class ContextConfig
    {
        public:

            ContextConfig(
                    _In_ uint32_t guid,
                    _In_ const std::string& name,
                    _In_ const std::string& dbAsic,
                    _In_ const std::string& dbCounters,
                    _In_ const std::string& dbFlex,
                    _In_ const std::string& dbState);

            virtual ~ContextConfig();

        public:

            void insert(
                    _In_ std::shared_ptr<LinecardConfig> config);


        public: // TODO to private

            uint32_t m_guid;

            std::string m_name;

            std::string m_dbAsic;

            std::string m_dbCounters;

            std::string m_dbFlex;   // TODO to be removed since only used to subscribe

            std::string m_dbState;

            std::shared_ptr<LinecardConfigContainer> m_scc;
    };
}
