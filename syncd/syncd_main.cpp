#include "unistd.h"
#include "otairediscommon.h"

#include "CommandLineOptionsParser.h"
#include "Syncd.h"
#include "MetadataLogger.h"

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

    SWSS_LOG_WARN("--- Starting Sync Daemon ---");

    auto vendorOtai = std::make_shared<VendorOtai>();

    auto syncd = std::make_shared<Syncd>(vendorOtai, commandLineOptions);

    syncd->run();

    SWSS_LOG_WARN("--- Stopping Sync Daemon ---");

    return EXIT_SUCCESS;
}
