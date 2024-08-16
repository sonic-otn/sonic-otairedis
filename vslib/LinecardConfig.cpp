#include "otaivs.h"
#include "LinecardConfig.h"

#include "swss/logger.h"

using namespace otaivs;

LinecardConfig::LinecardConfig():
    m_otaiLinecardType("OTN"),
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

    if (st == OTAI_VALUE_OTAI_LINECARD_TYPE_OTN)
    {
        otaiLinecardType = "OTN";
    }
    else
    {
        SWSS_LOG_ERROR("unknown OTAI linecard type: '%s'",otaiLinecardTypeStr);
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

    if (st == OTAI_VALUE_VS_LINECARD_TYPE_OTN)
    {
        linecardType = OTAI_VS_LINECARD_TYPE_OTN;
    }
    else
    {
        SWSS_LOG_ERROR("unknown linecard type: '%s', expected (%s)",
                linecardTypeStr,
                OTAI_VALUE_VS_LINECARD_TYPE_OTN);

        return false;
    }

    return true;
}
