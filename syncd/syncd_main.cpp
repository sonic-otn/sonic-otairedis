#include "unistd.h"
#include "lairediscommon.h"

#include "CommandLineOptionsParser.h"
#include "Syncd.h"
#include "MetadataLogger.h"

#ifdef LAITHRIFT
#include <utility>
#include <algorithm>
#include <linecard_lai_rpc_server.h>
#endif // LAITHRIFT

#ifdef LAITHRIFT
#define LINECARD_LAI_THRIFT_RPC_SERVER_PORT 9092
#endif // LAITHRIFT

using namespace syncd;

/*
 * Make sure that notification queue pointer is populated before we start
 * thread, and before we create_linecard, since at linecard_create we can start
 * receiving fdb_notifications which will arrive on different thread and
 * will call getQueueSize() when queue pointer could be null (this=0x0).
 */

int syncd_main(int argc, char **argv)
{
    swss::Logger::getInstance().setMinPrio(swss::Logger::SWSS_DEBUG);

    SWSS_LOG_ENTER();

    swss::Logger::getInstance().setMinPrio(swss::Logger::SWSS_NOTICE);

    swss::Logger::linkToDbNative("syncd");

    MetadataLogger::initialize();

    auto commandLineOptions = CommandLineOptionsParser::parseCommandLine(argc, argv);

#ifdef LAITHRIFT

    if (commandLineOptions->m_runRPCServer)
    {
        start_lai_thrift_rpc_server(LINECARD_LAI_THRIFT_RPC_SERVER_PORT);

        SWSS_LOG_NOTICE("rpcserver started");
    }

#endif // LAITHRIFT

    SWSS_LOG_WARN("--- Starting Sync Daemon ---");

    auto vendorLai = std::make_shared<VendorLai>();

    auto syncd = std::make_shared<Syncd>(vendorLai, commandLineOptions, true);

    syncd->run();

    SWSS_LOG_WARN("--- Stopping Sync Daemon ---");

    return EXIT_SUCCESS;
}
