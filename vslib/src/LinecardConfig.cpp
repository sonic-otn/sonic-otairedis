#include "laivs.h"
#include "LinecardConfig.h"

#include "swss/logger.h"

using namespace laivs;

LinecardConfig::LinecardConfig():
    m_laiLinecardType(LAI_LINECARD_TYPE_P230C),
    m_linecardType(LAI_VS_LINECARD_TYPE_NONE),
    m_bootType(LAI_VS_BOOT_TYPE_COLD),
    m_linecardIndex(0),
    m_hardwareInfo(""),
    m_useTapDevice(false)
{
    SWSS_LOG_ENTER();

    // empty
}


bool LinecardConfig::parseLaiLinecardType(
        _In_ const char* laiLinecardTypeStr,
        _Out_ lai_linecard_type_t& laiLinecardType)
{
    SWSS_LOG_ENTER();

    std::string st = (laiLinecardTypeStr == NULL) ? "unknown" : laiLinecardTypeStr;

    if (st == LAI_VALUE_LAI_LINECARD_TYPE_P230C)
    {
        laiLinecardType = LAI_LINECARD_TYPE_P230C;
    }
    else
    {
        SWSS_LOG_ERROR("unknown LAI linecard type: '%s', expected (%s|%s)",
                laiLinecardTypeStr,
                LAI_VALUE_LAI_LINECARD_TYPE_NPU,
                LAI_VALUE_LAI_LINECARD_TYPE_PHY);

        return false;
    }

    return true;
}

bool LinecardConfig::parseLinecardType(
        _In_ const char* linecardTypeStr,
        _Out_ lai_vs_linecard_type_t& linecardType)
{
    SWSS_LOG_ENTER();

    std::string st = (linecardTypeStr == NULL) ? "unknown" : linecardTypeStr;

    if (st == LAI_VALUE_VS_LINECARD_TYPE_P230C)
    {
        linecardType = LAI_VS_LINECARD_TYPE_P230C;
    }
    else
    {
        SWSS_LOG_ERROR("unknown linecard type: '%s', expected (%s)",
                linecardTypeStr,
                LAI_VALUE_VS_LINECARD_TYPE_P230C);

        return false;
    }

    return true;
}

bool LinecardConfig::parseBootType(
        _In_ const char* bootTypeStr,
        _Out_ lai_vs_boot_type_t& bootType)
{
    SWSS_LOG_ENTER();

    std::string bt = (bootTypeStr == NULL) ? "cold" : bootTypeStr;

    if (bt == "cold" || bt == LAI_VALUE_VS_BOOT_TYPE_COLD)
    {
        bootType = LAI_VS_BOOT_TYPE_COLD;
    }
    else
    {
        SWSS_LOG_ERROR("unknown boot type: '%s', expected (cold)", bootTypeStr);

        return false;
    }

    return true;
}

bool LinecardConfig::parseUseTapDevice(
        _In_ const char* useTapDeviceStr)
{
    SWSS_LOG_ENTER();

    if (useTapDeviceStr)
    {
        std::string utd = useTapDeviceStr;

        return utd == "true";
    }

    return false;
}
