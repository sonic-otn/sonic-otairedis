#pragma once

#include "swss/sal.h"

#include <string>

namespace syncd
{
    typedef enum _syncd_restart_type_t
    {
        SYNCD_RESTART_TYPE_COLD,

    } syncd_restart_type_t;

    class RequestShutdownCommandLineOptions
    {
        public:

            RequestShutdownCommandLineOptions();

            virtual ~RequestShutdownCommandLineOptions();

        public:

            syncd_restart_type_t getRestartType() const;

            void setRestartType(
                    _In_ syncd_restart_type_t restartType);

            static syncd_restart_type_t stringToRestartType(
                    _In_ const std::string& restartType);

            static std::string restartTypeToString(
                    _In_ syncd_restart_type_t restartType);

        private:

            syncd_restart_type_t m_restartType;

        public:

            uint32_t m_globalContext;

            std::string m_contextConfig;
    };
}
