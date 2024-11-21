#include "CommandLineOptionsParser.h"

#include "meta/otai_serialize.h"

#include "swss/logger.h"

#include <getopt.h>

#include <iostream>

using namespace syncd;

std::shared_ptr<CommandLineOptions> CommandLineOptionsParser::parseCommandLine(
        _In_ int argc,
        _In_ char **argv)
{
    SWSS_LOG_ENTER();

    auto options = std::make_shared<CommandLineOptions>();
    const char* const optstring = "p:f:lh";

    while (true)
    {
        static struct option long_options[] =
        {
            { "profile",                 required_argument, 0, 'p' },
            { "enableOtaiBulkSupport",    no_argument,       0, 'l' },
            { "help",                    no_argument,       0, 'h' },
            { 0,                         0,                 0,  0  }
        };

        int option_index = 0;

        int c = getopt_long(argc, argv, optstring, long_options, &option_index);

        if (c == -1)
        {
            break;
        }

        switch (c)
        {
            case 'p':
                options->m_profileMapFile = std::string(optarg);
                break;

            case 'l':
                options->m_enableOtaiBulkSupport = true;
                break;

            case 'h':
                printUsage();
                exit(EXIT_SUCCESS);

            case '?':
                SWSS_LOG_WARN("unknown option %c", optopt);
                printUsage();
                exit(EXIT_FAILURE);


            default:
                SWSS_LOG_ERROR("getopt_long failure");
                exit(EXIT_FAILURE);
        }
    }

    return options;
}

void CommandLineOptionsParser::printUsage()
{
    SWSS_LOG_ENTER();
    std::cout << "Usage: syncd [-p profile] [-l] [-h]" << std::endl;
    std::cout << "    -p --profile profile" << std::endl;
    std::cout << "        Provide profile map file" << std::endl;
    std::cout << "    -l --enableBulk" << std::endl;
    std::cout << "        Enable OTAI Bulk support" << std::endl;
    std::cout << "    -h --help" << std::endl;
    std::cout << "        Print out this message" << std::endl;
}
