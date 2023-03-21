#include "RequestShutdownCommandLineOptionsParser.h"

#include "swss/logger.h"

#include <unistd.h>
#include <getopt.h>

#include <ostream>
#include <iostream>

using namespace syncd;

std::shared_ptr<RequestShutdownCommandLineOptions> RequestShutdownCommandLineOptionsParser::parseCommandLine(
        _In_ int argc,
        _In_ char **argv)
{
    SWSS_LOG_ENTER();

    auto options = std::make_shared<RequestShutdownCommandLineOptions>();

    static struct option long_options[] =
    {
        { "cold", no_argument, 0, 'c' },
        { "globalContext", required_argument, 0, 'g' },
        { "contextContig", required_argument, 0, 'x' },
    };

    bool optionSpecified = false;

    while (true)
    {
        int option_index = 0;

        int c = getopt_long(argc, argv, "cg:x:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 'c':
                options->setRestartType(SYNCD_RESTART_TYPE_COLD);
                optionSpecified = true;
                break;

            case 'g':
                options->m_globalContext = (uint32_t)std::stoul(optarg);
                break;

            case 'x':
                options->m_contextConfig = std::string(optarg);
                break;

            default:
                SWSS_LOG_ERROR("getopt failure");
                exit(EXIT_FAILURE);
        }
    }

    if (!optionSpecified)
    {
        SWSS_LOG_ERROR("no shutdown option specified");

        printUsage();

        exit(EXIT_FAILURE);
    }

    return options;
}

void RequestShutdownCommandLineOptionsParser::printUsage()
{
    SWSS_LOG_ENTER();

    std::cout << "Usage: syncd_request_shutdown [-c] [--cold] [-g idx] [-x contextConfig]" << std::endl;

    std::cerr << std::endl;

    std::cerr << "Shutdown option must be specified" << std::endl;
    std::cerr << "---------------------------------" << std::endl;
    std::cerr << " --cold  -c   for cold restart" << std::endl;
    std::cerr << std::endl;
    std::cout << " --globalContext  -g" << std::endl;
    std::cout << "              Global context index to load from context config file" << std::endl;
    std::cout << " --contextConfig  -x" << std::endl;
    std::cout << "              Context configuration file" << std::endl;
}
