#pragma once

extern "C" {
#include "otaimetadata.h"
}

#include "swss/logger.h"

#include <memory>
#include <string>

namespace otaimeta
{
    class Globals
    {
        private:

            Globals() = delete;
            ~Globals() = delete;

        public:

            static std::string getAttrInfo(
                    _In_ const otai_attr_metadata_t& md);
    };
}

#define META_LOG_WARN(   md, format, ...)   SWSS_LOG_WARN   ("%s " format, otaimeta::Globals::getAttrInfo(md).c_str(), ##__VA_ARGS__)
#define META_LOG_ERROR(  md, format, ...)   SWSS_LOG_ERROR  ("%s " format, otaimeta::Globals::getAttrInfo(md).c_str(), ##__VA_ARGS__)
#define META_LOG_DEBUG(  md, format, ...)   SWSS_LOG_DEBUG  ("%s " format, otaimeta::Globals::getAttrInfo(md).c_str(), ##__VA_ARGS__)
#define META_LOG_NOTICE( md, format, ...)   SWSS_LOG_NOTICE ("%s " format, otaimeta::Globals::getAttrInfo(md).c_str(), ##__VA_ARGS__)
#define META_LOG_INFO(   md, format, ...)   SWSS_LOG_INFO   ("%s " format, otaimeta::Globals::getAttrInfo(md).c_str(), ##__VA_ARGS__)
#define META_LOG_THROW(  md, format, ...)   SWSS_LOG_THROW  ("%s " format, otaimeta::Globals::getAttrInfo(md).c_str(), ##__VA_ARGS__)

