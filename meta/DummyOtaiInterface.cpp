#include "DummyOtaiInterface.h"

#include "swss/logger.h"

#include <memory>

using namespace otaimeta;

DummyOtaiInterface::DummyOtaiInterface()
{
    SWSS_LOG_ENTER();

    m_status = OTAI_STATUS_SUCCESS;
}

void DummyOtaiInterface::setStatus(
        _In_ otai_status_t status)
{
    SWSS_LOG_ENTER();

    m_status = status;
}

otai_status_t DummyOtaiInterface::initialize(
        _In_ uint64_t flags,
        _In_ const otai_service_method_table_t *service_method_table)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_SUCCESS;
}

otai_status_t  DummyOtaiInterface::uninitialize(void)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_SUCCESS;
}

otai_status_t  DummyOtaiInterface::linkCheck(void)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_SUCCESS;
}

otai_status_t DummyOtaiInterface::create(
        _In_ otai_object_type_t objectType,
        _Out_ otai_object_id_t* objectId,
        _In_ otai_object_id_t linecardId,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return m_status;
}

otai_status_t DummyOtaiInterface::remove(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return m_status;
}

otai_status_t DummyOtaiInterface::set(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ const otai_attribute_t *attr)
{
    SWSS_LOG_ENTER();

    return m_status;
}

otai_status_t DummyOtaiInterface::get(
        _In_ otai_object_type_t objectType,
        _In_ otai_object_id_t objectId,
        _In_ uint32_t attr_count,
        _Inout_ otai_attribute_t *attr_list)
{
    SWSS_LOG_ENTER();

    return m_status;
}

otai_status_t DummyOtaiInterface::objectTypeGetAvailability(
        _In_ otai_object_id_t linecardId,
        _In_ otai_object_type_t objectType,
        _In_ uint32_t attrCount,
        _In_ const otai_attribute_t *attrList,
        _Out_ uint64_t *count)
{
    SWSS_LOG_ENTER();

    return m_status;
}

otai_status_t DummyOtaiInterface::queryAttributeCapability(
        _In_ otai_object_id_t linecardId,
        _In_ otai_object_type_t objectType,
        _In_ otai_attr_id_t attrId,
        _Out_ otai_attr_capability_t *capability)
{
    SWSS_LOG_ENTER();

    return m_status;
}

otai_status_t DummyOtaiInterface::queryAattributeEnumValuesCapability(
        _In_ otai_object_id_t linecardId,
        _In_ otai_object_type_t objectType,
        _In_ otai_attr_id_t attrId,
        _Inout_ otai_s32_list_t *enumValuesCapability)
{
    SWSS_LOG_ENTER();

    return m_status;
}

otai_object_type_t DummyOtaiInterface::objectTypeQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (m_status != OTAI_STATUS_SUCCESS)
        SWSS_LOG_THROW("not implemented");

    return OTAI_OBJECT_TYPE_NULL;
}

otai_object_id_t DummyOtaiInterface::linecardIdQuery(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    if (m_status != OTAI_STATUS_SUCCESS)
        SWSS_LOG_THROW("not implemented");

    return OTAI_NULL_OBJECT_ID;
}

otai_status_t DummyOtaiInterface::logSet(
        _In_ otai_api_t api,
        _In_ otai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    return m_status;
}
