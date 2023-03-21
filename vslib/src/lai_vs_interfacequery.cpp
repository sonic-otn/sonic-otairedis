#include "lai_vs.h"

using namespace laivs;

std::shared_ptr<Lai> vs_lai = std::make_shared<Lai>();

lai_status_t lai_api_initialize(
        _In_ uint64_t flags,
        _In_ const lai_service_method_table_t *service_method_table)
{
    SWSS_LOG_ENTER();

    return vs_lai->initialize(flags, service_method_table);
}

lai_status_t lai_api_uninitialize(void)
{
    SWSS_LOG_ENTER();

    return vs_lai->uninitialize();
}

lai_status_t lai_log_set(
        _In_ lai_api_t lai_api_id,
        _In_ lai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    return LAI_STATUS_NOT_IMPLEMENTED;
}

#define API(api) .api ## _api = const_cast<lai_ ## api ## _api_t*>(&vs_ ## api ## _api)

static lai_apis_t vs_apis = {
    API(linecard),
    API(port),
    API(transceiver),
    API(logicalchannel),
    API(otn),
    API(ethernet),
    API(physicalchannel),
    API(och),
    API(lldp),
    API(assignment),
    API(interface),
    API(oa),
    API(osc),
    API(aps),
    API(apsport),
    API(attenuator),
    API(wss),
    API(mediachannel),
    API(ocm),
    API(otdr),
};

static_assert((sizeof(lai_apis_t)/sizeof(void*)) == (LAI_API_EXTENSIONS_MAX - 1), "LAI API size error");

lai_status_t lai_api_query(
        _In_ lai_api_t lai_api_id,
        _Out_ void** api_method_table)
{
    SWSS_LOG_ENTER();

    if (api_method_table == NULL)
    {
        SWSS_LOG_ERROR("NULL method table passed to LAI API initialize");
        return LAI_STATUS_INVALID_PARAMETER;
    }

    if (lai_metadata_get_enum_value_name(&lai_metadata_enum_lai_api_t, lai_api_id))
    {
        *api_method_table = ((void**)&vs_apis)[lai_api_id - 1];
        return LAI_STATUS_SUCCESS;
    }

    SWSS_LOG_ERROR("Invalid API type %d", lai_api_id);

    return LAI_STATUS_INVALID_PARAMETER;
}

lai_status_t lai_link_check(_Out_ bool *up)
{
    SWSS_LOG_ENTER();

    return vs_lai->linkCheck(up);
}

lai_status_t lai_query_attribute_capability(
        _In_ lai_object_id_t linecard_id,
        _In_ lai_object_type_t object_type,
        _In_ lai_attr_id_t attr_id,
        _Out_ lai_attr_capability_t *capability)
{
    SWSS_LOG_ENTER();

    return vs_lai->queryAttributeCapability(
            linecard_id,
            object_type,
            attr_id,
            capability);
}

lai_status_t lai_query_attribute_enum_values_capability(
        _In_ lai_object_id_t linecard_id,
        _In_ lai_object_type_t object_type,
        _In_ lai_attr_id_t attr_id,
        _Inout_ lai_s32_list_t *enum_values_capability)
{
    SWSS_LOG_ENTER();

    return vs_lai->queryAattributeEnumValuesCapability(
            linecard_id,
            object_type,
            attr_id,
            enum_values_capability);
}

lai_status_t lai_object_type_get_availability(
        _In_ lai_object_id_t linecard_id,
        _In_ lai_object_type_t object_type,
        _In_ uint32_t attr_count,
        _In_ const lai_attribute_t *attr_list,
        _Out_ uint64_t *count)
{
    SWSS_LOG_ENTER();

    return vs_lai->objectTypeGetAvailability(
            linecard_id,
            object_type,
            attr_count,
            attr_list,
            count);
}

lai_object_type_t lai_object_type_query(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return vs_lai->objectTypeQuery(objectId);
}

lai_object_id_t lai_linecard_id_query(
        _In_ lai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return vs_lai->linecardIdQuery(objectId);
}
