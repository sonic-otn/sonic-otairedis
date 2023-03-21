#include "CommandLineOptions.h"

#include "meta/lai_serialize.h"

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
    m_enableLaiBulkSupport = false;

    m_redisCommunicationMode = LAI_REDIS_COMMUNICATION_MODE_REDIS_ASYNC;

    m_loglevel = swss::Logger::SWSS_INFO;

    m_profileMapFile = "";

    m_globalContext = 0;

    m_contextConfig = "";

#ifdef LAITHRIFT

    m_runRPCServer = false;

#endif // LAITHRIFT

}

std::string CommandLineOptions::getCommandLineString() const
{
    SWSS_LOG_ENTER();

    std::stringstream ss;

    ss << " EnableDiagShell=" << (m_enableDiagShell ? "YES" : "NO");
    ss << " EnableUnittests=" << (m_enableUnittests ? "YES" : "NO");
    ss << " EnableConsistencyCheck=" << (m_enableConsistencyCheck ? "YES" : "NO");
    ss << " EnableSyncMode=" << (m_enableSyncMode ? "YES" : "NO");
    ss << " RedisCommunicationMode=" << lai_serialize_redis_communication_mode(m_redisCommunicationMode);
    ss << " EnableLaiBulkSuport=" << (m_enableLaiBulkSupport ? "YES" : "NO");
    ss << " ProfileMapFile=" << m_profileMapFile;
    ss << " GlobalContext=" << m_globalContext;
    ss << " ContextConfig=" << m_contextConfig;
#ifdef LAITHRIFT

    ss << " RunRPCServer=" << (m_runRPCServer ? "YES" : "NO");

#endif // LAITHRIFT

    return ss.str();
}

