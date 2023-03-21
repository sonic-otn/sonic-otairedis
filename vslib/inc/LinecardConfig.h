#pragma once

#include "LaneMap.h"
#include "EventQueue.h"
#include "ResourceLimiter.h"
#include "CorePortIndexMap.h"

#include <string>
#include <memory>

extern "C" {
#include "lai.h"     // for lai_linecard_type_t
}

namespace laivs
{
    typedef enum _lai_vs_linecard_type_t
    {
        LAI_VS_LINECARD_TYPE_NONE,

        LAI_VS_LINECARD_TYPE_P230C,

    } lai_vs_linecard_type_t;

    typedef enum _lai_vs_boot_type_t
    {
        LAI_VS_BOOT_TYPE_COLD,

        LAI_VS_BOOT_TYPE_WARM,

        LAI_VS_BOOT_TYPE_FAST,

    } lai_vs_boot_type_t;

    class LinecardConfig
    {
        public:

            LinecardConfig();

            virtual ~LinecardConfig() = default;

        public:

            static bool parseLaiLinecardType(
                    _In_ const char* laiLinecardTypeStr,
                    _Out_ lai_linecard_type_t& laiLinecardType);

            static bool parseLinecardType(
                    _In_ const char* linecardTypeStr,
                    _Out_ lai_vs_linecard_type_t& linecardType);

            static bool parseBootType(
                    _In_ const char* bootTypeStr,
                    _Out_ lai_vs_boot_type_t& bootType);

            static bool parseUseTapDevice(
                    _In_ const char* useTapDeviceStr);

        public:

            lai_linecard_type_t m_laiLinecardType;

            lai_vs_linecard_type_t m_linecardType;

            lai_vs_boot_type_t m_bootType;

            uint32_t m_linecardIndex;

            std::string m_hardwareInfo;

            bool m_useTapDevice;

            std::shared_ptr<LaneMap> m_laneMap;

            std::shared_ptr<LaneMap> m_fabricLaneMap;

            std::shared_ptr<EventQueue> m_eventQueue;

            std::shared_ptr<ResourceLimiter> m_resourceLimiter;

            std::shared_ptr<CorePortIndexMap> m_corePortIndexMap;
    };
}
