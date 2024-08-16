#include "CommandLineOptions.h"

#include "meta/otai_serialize.h"

#include "swss/logger.h"

#include <sstream>

using namespace syncd;

CommandLineOptions::CommandLineOptions()
{
    SWSS_LOG_ENTER();

    // default values for command line options

    m_enableSyncMode = false;
    m_enableOtaiBulkSupport = false;

    m_redisCommunicationMode = OTAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC;

    m_loglevel = swss::Logger::SWSS_INFO;

    m_profileMapFile = "";

    m_globalContext = 0;

    m_contextConfig = "";
}

std::string CommandLineOptions::getCommandLineString() const
{
    SWSS_LOG_ENTER();

    std::stringstream ss;

    ss << " EnableSyncMode=" << (m_enableSyncMode ? "YES" : "NO");
    ss << " RedisCommunicationMode=" << otai_serialize_redis_communication_mode(m_redisCommunicationMode);
    ss << " EnableOtaiBulkSuport=" << (m_enableOtaiBulkSupport ? "YES" : "NO");
    ss << " ProfileMapFile=" << m_profileMapFile;
    ss << " GlobalContext=" << m_globalContext;
    ss << " ContextConfig=" << m_contextConfig;

    return ss.str();
}

