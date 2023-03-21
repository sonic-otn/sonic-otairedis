#include "LinecardConfig.h"

#include "swss/logger.h"

using namespace lairedis;

LinecardConfig::LinecardConfig():
    m_linecardIndex(0),
    m_hardwareInfo("")
{
    SWSS_LOG_ENTER();

    // empty
}

LinecardConfig::LinecardConfig(
        _In_ uint32_t linecardIndex,
        _In_ const std::string& hwinfo):
    m_linecardIndex(linecardIndex),
    m_hardwareInfo(hwinfo)
{
    SWSS_LOG_ENTER();

    // empty
}
