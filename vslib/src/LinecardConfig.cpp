#include "otaivs.h"
#include "LinecardConfig.h"

#include "swss/logger.h"

using namespace otaivs;

LinecardConfig::LinecardConfig():
    m_otaiLinecardType("P230C"),
    m_linecardType(OTAI_VS_LINECARD_TYPE_NONE),
    m_bootType(OTAI_VS_BOOT_TYPE_COLD),
    m_linecardIndex(0),
    m_hardwareInfo(""),
    m_useTapDevice(false)
{
    SWSS_LOG_ENTER();

    // empty
}


bool LinecardConfig::parseOtaiLinecardType(
        _In_ const char* otaiLinecardTypeStr,
        _Out_ std::string& otaiLinecardType)
{
    SWSS_LOG_ENTER();

    std::string st = (otaiLinecardTypeStr == NULL) ? "unknown" : otaiLinecardTypeStr;

    if (st == OTAI_VALUE_OTAI_LINECARD_TYPE_P230C)
    {
        otaiLinecardType = "P230C";
    }
    else
    {
        SWSS_LOG_ERROR("unknown OTAI linecard type: '%s', expected (%s|%s)",
                otaiLinecardTypeStr,
                OTAI_VALUE_OTAI_LINECARD_TYPE_NPU,
                OTAI_VALUE_OTAI_LINECARD_TYPE_PHY);

        return false;
    }

    return true;
}

bool LinecardConfig::parseLinecardType(
        _In_ const char* linecardTypeStr,
        _Out_ otai_vs_linecard_type_t& linecardType)
{
    SWSS_LOG_ENTER();

    std::string st = (linecardTypeStr == NULL) ? "unknown" : linecardTypeStr;

    if (st == OTAI_VALUE_VS_LINECARD_TYPE_P230C)
    {
        linecardType = OTAI_VS_LINECARD_TYPE_P230C;
    }
    else
    {
        SWSS_LOG_ERROR("unknown linecard type: '%s', expected (%s)",
                linecardTypeStr,
                OTAI_VALUE_VS_LINECARD_TYPE_P230C);

        return false;
    }

    return true;
}

bool LinecardConfig::parseBootType(
        _In_ const char* bootTypeStr,
        _Out_ otai_vs_boot_type_t& bootType)
{
    SWSS_LOG_ENTER();

    std::string bt = (bootTypeStr == NULL) ? "cold" : bootTypeStr;

    if (bt == "cold" || bt == OTAI_VALUE_VS_BOOT_TYPE_COLD)
    {
        bootType = OTAI_VS_BOOT_TYPE_COLD;
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
