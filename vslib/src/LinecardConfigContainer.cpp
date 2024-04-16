#include "LinecardConfigContainer.h"

#include "swss/logger.h"

using namespace otaivs;

void LinecardConfigContainer::insert(
        _In_ std::shared_ptr<LinecardConfig> config)
{
    SWSS_LOG_ENTER();

    auto it = m_indexToConfig.find(config->m_linecardIndex);

    if (it != m_indexToConfig.end())
    {
        SWSS_LOG_THROW("linecard with index %u already exists", config->m_linecardIndex);
    }

    auto itt = m_hwinfoToConfig.find(config->m_hardwareInfo);

    if (itt != m_hwinfoToConfig.end())
    {
        SWSS_LOG_THROW("linecard with hwinfo '%s' already exists",
                config->m_hardwareInfo.c_str());
    }

    SWSS_LOG_NOTICE("added linecard: %u:%s",
            config->m_linecardIndex,
            config->m_hardwareInfo.c_str());

    m_indexToConfig[config->m_linecardIndex] = config;

    m_hwinfoToConfig[config->m_hardwareInfo] = config;
}

std::shared_ptr<LinecardConfig> LinecardConfigContainer::getConfig(
        _In_ uint32_t linecardIndex) const
{
    SWSS_LOG_ENTER();

    auto it = m_indexToConfig.find(linecardIndex);

    if (it != m_indexToConfig.end())
    {
        return it->second;
    }

    return nullptr;
}

std::shared_ptr<LinecardConfig> LinecardConfigContainer::getConfig(
        _In_ const std::string& hardwareInfo) const
{
    SWSS_LOG_ENTER();

    auto it = m_hwinfoToConfig.find(hardwareInfo);

    if (it != m_hwinfoToConfig.end())
    {
        return it->second;
    }

    return nullptr;
}
