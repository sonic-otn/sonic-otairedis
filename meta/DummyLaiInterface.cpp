#include "DummyLaiInterface.h"

#include "swss/logger.h"

#include <memory>

using namespace laimeta;

DummyLaiInterface::DummyLaiInterface()
{
    SWSS_LOG_ENTER();

    m_status = LAI_STATUS_SUCCESS;
}

void DummyLaiInterface::setStatus(
        _In_ lai_status_t status)
{
    SWSS_LOG_ENTER();

    m_status = status;
}

lai_status_t DummyLaiInterface::initialize(
        _In_ uint64_t flags,
        _In_ const lai_service_method_table_t *service_method_table)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_SUCCESS;
}

lai_status_t  DummyLaiInterface::uninitialize(void)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_SUCCESS;
}

lai_status_t  DummyLaiInterface::linkCheck(void)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_SUCCESS;
}

lai_status_t DummyLaiInterface::create(
        _In_ lai_object_type_t objectType,
        _Out_ lai_object_id_t* objectId,
        _In_ lai_object_id_t linecardId,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return m_status;
}

lai_status_t DummyLaiInterface::remove(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_status;
}

lai_status_t DummyLaiInterface::set(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId,
        _In_ const lai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    return m_status;
}

lai_status_t DummyLaiInterface::get(
        _In_ lai_object_type_t objectType,
        _In_ lai_object_id_t objectId,
        _In_ uint32_t attr_count,
        _Inout_ lai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return m_status;
}

lai_status_t DummyLaiInterface::objectTypeGetAvailability(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ uint32_t attrCount,
        _In_ const lai_attribute_t *attrList,
        _Out_ uint64_t *count)
{
    SWSS_LOG_ENTER();

    return m_status;
}

lai_status_t DummyLaiInterface::queryAttributeCapability(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ lai_attr_id_t attrId,
        _Out_ lai_attr_capability_t *capability)
{
    SWSS_LOG_ENTER();

    return m_status;
}

lai_status_t DummyLaiInterface::queryAattributeEnumValuesCapability(
        _In_ lai_object_id_t linecardId,
        _In_ lai_object_type_t objectType,
        _In_ lai_attr_id_t attrId,
        _Inout_ lai_s32_list_t *enumValuesCapability)
{
    SWSS_LOG_ENTER();

    return m_status;
}

lai_object_type_t DummyLaiInterface::objectTypeQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (m_status != LAI_STATUS_SUCCESS)
        SWSS_LOG_THROW("not implemented");

    return LAI_OBJECT_TYPE_NULL;
}

lai_object_id_t DummyLaiInterface::linecardIdQuery(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (m_status != LAI_STATUS_SUCCESS)
        SWSS_LOG_THROW("not implemented");

    return LAI_NULL_OBJECT_ID;
}

lai_status_t DummyLaiInterface::logSet(
        _In_ lai_api_t api,
        _In_ lai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    return m_status;
}
