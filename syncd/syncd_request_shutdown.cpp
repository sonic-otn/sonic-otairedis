#include "RequestShutdown.h"

#include "swss/logger.h"

using namespace syncd;

int main(int argc, char **argv)
{
    swss::Logger::getInstance().setMinPrio(swss::Logger::SWSS_NOTICE);

    SWSS_LOG_ENTER();

    RequestShutdown rs;

    rs.send();

    return EXIT_SUCCESS;
}
