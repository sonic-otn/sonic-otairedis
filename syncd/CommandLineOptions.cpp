#include "CommandLineOptions.h"

#include "meta/otai_serialize.h"

#include "swss/logger.h"

#include <sstream>

using namespace syncd;

CommandLineOptions::CommandLineOptions()
{
    SWSS_LOG_ENTER();

    // default values for command line options

    m_enableDiagShell = false;
    m_enableUnittests = false;
    m_enableConsistencyCheck = false;
    m_enableSyncMode = false;
    m_enableOtaiBulkSupport = false;

    m_redisCommunicationMode = OTAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC;

    m_loglevel = swss::Logger::SWSS_INFO;

    m_profileMapFile = "";

    m_globalContext = 0;

    m_contextConfig = "";

#ifdef OTAITHRIFT

    m_runRPCServer = false;

#endif // OTAITHRIFT

}

std::string CommandLineOptions::getCommandLineString() const
{
    SWSS_LOG_ENTER();

    std::stringstream ss;

    ss << " EnableDiagShell=" << (m_enableDiagShell ? "YES" : "NO");
    ss << " EnableUnittests=" << (m_enableUnittests ? "YES" : "NO");
    ss << " EnableConsistencyCheck=" << (m_enableConsistencyCheck ? "YES" : "NO");
    ss << " EnableSyncMode=" << (m_enableSyncMode ? "YES" : "NO");
    ss << " RedisCommunicationMode=" << otai_serialize_redis_communication_mode(m_redisCommunicationMode);
    ss << " EnableOtaiBulkSuport=" << (m_enableOtaiBulkSupport ? "YES" : "NO");
    ss << " ProfileMapFile=" << m_profileMapFile;
    ss << " GlobalContext=" << m_globalContext;
    ss << " ContextConfig=" << m_contextConfig;
#ifdef OTAITHRIFT

    ss << " RunRPCServer=" << (m_runRPCServer ? "YES" : "NO");

#endif // OTAITHRIFT

    return ss.str();
}

