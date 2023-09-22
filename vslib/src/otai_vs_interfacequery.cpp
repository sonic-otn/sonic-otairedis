#include "otai_vs.h"

using namespace otaivs;

std::shared_ptr<Otai> vs_otai = std::make_shared<Otai>();

otai_status_t otai_api_initialize(
        _In_ uint64_t flags,
        _In_ const otai_service_method_table_t *service_method_table)
{
    SWSS_LOG_ENTER();

    return vs_otai->initialize(flags, service_method_table);
}

otai_status_t otai_api_uninitialize(void)
{
    SWSS_LOG_ENTER();

    return vs_otai->uninitialize();
}

otai_status_t otai_log_set(
        _In_ otai_api_t otai_api_id,
        _In_ otai_log_level_t log_level)
{
    SWSS_LOG_ENTER();

    return OTAI_STATUS_NOT_IMPLEMENTED;
}

#define API(api) .api ## _api = const_cast<otai_ ## api ## _api_t*>(&vs_ ## api ## _api)

static otai_apis_t vs_apis = {
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

static_assert((sizeof(otai_apis_t)/sizeof(void*)) == (OTAI_API_EXTENSIONS_MAX - 1), "OTAI API size error");

otai_status_t otai_api_query(
        _In_ otai_api_t otai_api_id,
        _Out_ void** api_method_table)
{
    SWSS_LOG_ENTER();

    if (api_method_table == NULL)
    {
        SWSS_LOG_ERROR("NULL method table passed to OTAI API initialize");
        return OTAI_STATUS_INVALID_PARAMETER;
    }

    if (otai_metadata_get_enum_value_name(&otai_metadata_enum_otai_api_t, otai_api_id))
    {
        *api_method_table = ((void**)&vs_apis)[otai_api_id - 1];
        return OTAI_STATUS_SUCCESS;
    }

    SWSS_LOG_ERROR("Invalid API type %d", otai_api_id);

    return OTAI_STATUS_INVALID_PARAMETER;
}

otai_status_t otai_link_check(_Out_ bool *up)
{
    SWSS_LOG_ENTER();

    return vs_otai->linkCheck(up);
}

otai_status_t otai_query_attribute_capability(
        _In_ otai_object_id_t linecard_id,
        _In_ otai_object_type_t object_type,
        _In_ otai_attr_id_t attr_id,
        _Out_ otai_attr_capability_t *capability)
{
    SWSS_LOG_ENTER();

    return vs_otai->queryAttributeCapability(
            linecard_id,
            object_type,
            attr_id,
            capability);
}

otai_status_t otai_query_attribute_enum_values_capability(
        _In_ otai_object_id_t linecard_id,
        _In_ otai_object_type_t object_type,
        _In_ otai_attr_id_t attr_id,
        _Inout_ otai_s32_list_t *enum_values_capability)
{
    SWSS_LOG_ENTER();

    return vs_otai->queryAattributeEnumValuesCapability(
            linecard_id,
            object_type,
            attr_id,
            enum_values_capability);
}

otai_status_t otai_object_type_get_availability(
        _In_ otai_object_id_t linecard_id,
        _In_ otai_object_type_t object_type,
        _In_ uint32_t attr_count,
        _In_ const otai_attribute_t *attr_list,
        _Out_ uint64_t *count)
{
    SWSS_LOG_ENTER();

    return vs_otai->objectTypeGetAvailability(
            linecard_id,
            object_type,
            attr_count,
            attr_list,
            count);
}

otai_object_type_t otai_object_type_query(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return vs_otai->objectTypeQuery(objectId);
}

otai_object_id_t otai_linecard_id_query(
        _In_ otai_object_id_t objectId)
{
    SWSS_LOG_ENTER();

    return vs_otai->linecardIdQuery(objectId);
}
