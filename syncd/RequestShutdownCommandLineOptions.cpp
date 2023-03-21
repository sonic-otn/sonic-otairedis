#include "RequestShutdownCommandLineOptions.h"

#include "swss/logger.h"

using namespace syncd;

RequestShutdownCommandLineOptions::RequestShutdownCommandLineOptions():
    m_restartType(SYNCD_RESTART_TYPE_COLD)
{
    SWSS_LOG_ENTER();

    m_globalContext = 0;
}

RequestShutdownCommandLineOptions::~RequestShutdownCommandLineOptions()
{
    SWSS_LOG_ENTER();

    // empty
}

syncd_restart_type_t RequestShutdownCommandLineOptions::getRestartType() const
{
    SWSS_LOG_ENTER();

    return m_restartType;
}

void RequestShutdownCommandLineOptions::setRestartType(
        _In_ syncd_restart_type_t restartType)
{
    SWSS_LOG_ENTER();

    m_restartType = restartType;
}

#define STRING_RESTART_COLD         "COLD"

syncd_restart_type_t RequestShutdownCommandLineOptions::stringToRestartType(
        _In_ const std::string& restartType)
{
    SWSS_LOG_ENTER();

    if (restartType == STRING_RESTART_COLD)
        return SYNCD_RESTART_TYPE_COLD;

    SWSS_LOG_WARN("unknown restart type '%s' returning COLD", restartType.c_str());

    return SYNCD_RESTART_TYPE_COLD;
}

std::string RequestShutdownCommandLineOptions::restartTypeToString(
        _In_ syncd_restart_type_t restartType)
{
    SWSS_LOG_ENTER();

    switch (restartType)
    {
        case SYNCD_RESTART_TYPE_COLD:
            return STRING_RESTART_COLD;

        default:

            SWSS_LOG_WARN("unknown restart type '%d' returning COLD", restartType);
            return STRING_RESTART_COLD;
    }
}
