#pragma once

extern "C" {
#include "laimetadata.h"
}

#include "swss/logger.h"

#include <memory>
#include <string>

namespace laimeta
{
    class Globals
    {
        private:

            Globals() = delete;
            ~Globals() = delete;

        public:

            static std::string getAttrInfo(
                    _In_ const lai_attr_metadata_t& md);
    };
}

#define META_LOG_WARN(   md, format, ...)   SWSS_LOG_WARN   ("%s " format, laimeta::Globals::getAttrInfo(md).c_str(), ##__VA_ARGS__)
#define META_LOG_ERROR(  md, format, ...)   SWSS_LOG_ERROR  ("%s " format, laimeta::Globals::getAttrInfo(md).c_str(), ##__VA_ARGS__)
#define META_LOG_DEBUG(  md, format, ...)   SWSS_LOG_DEBUG  ("%s " format, laimeta::Globals::getAttrInfo(md).c_str(), ##__VA_ARGS__)
#define META_LOG_NOTICE( md, format, ...)   SWSS_LOG_NOTICE ("%s " format, laimeta::Globals::getAttrInfo(md).c_str(), ##__VA_ARGS__)
#define META_LOG_INFO(   md, format, ...)   SWSS_LOG_INFO   ("%s " format, laimeta::Globals::getAttrInfo(md).c_str(), ##__VA_ARGS__)
#define META_LOG_THROW(  md, format, ...)   SWSS_LOG_THROW  ("%s " format, laimeta::Globals::getAttrInfo(md).c_str(), ##__VA_ARGS__)

