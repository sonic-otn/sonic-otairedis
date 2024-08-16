#pragma once

#include "EventQueue.h"

#include <string>
#include <memory>

extern "C" {
#include "otai.h"
}

namespace otaivs
{
    typedef enum _otai_vs_linecard_type_t
    {
        OTAI_VS_LINECARD_TYPE_NONE,

        OTAI_VS_LINECARD_TYPE_OTN,

    } otai_vs_linecard_type_t;

    typedef enum _otai_vs_boot_type_t
    {
        OTAI_VS_BOOT_TYPE_COLD,

        OTAI_VS_BOOT_TYPE_WARM,

        OTAI_VS_BOOT_TYPE_FAST,

    } otai_vs_boot_type_t;

    class LinecardConfig
    {
        public:

            LinecardConfig();

            virtual ~LinecardConfig() = default;

        public:

            static bool parseOtaiLinecardType(
                    _In_ const char* otaiLinecardTypeStr,
                    _Out_ std::string& otaiLinecardType);

            static bool parseLinecardType(
                    _In_ const char* linecardTypeStr,
                    _Out_ otai_vs_linecard_type_t& linecardType);

        public:

            std::string m_otaiLinecardType;

            otai_vs_linecard_type_t m_linecardType;

            otai_vs_boot_type_t m_bootType;

            uint32_t m_linecardIndex;

            std::string m_hardwareInfo;

            bool m_useTapDevice;

            std::shared_ptr<EventQueue> m_eventQueue;

    };
}
