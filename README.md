
# OTAI Exention to SAI

## Instruction

1. Install required packages:
```
    sudo apt-get install doxygen perl graphviz
```
2. Include OTAI objects to SAI:
```
    python3 extension.py
```
3. Generate SAI metadata:
```
    cd SAI/meta
    make
```

If make success, you should see:
```
GNU Make 4.2.1
gcc (Ubuntu 10.5.0-1ubuntu1~20.04) 10.5.0
......
[ SAI_STATUS_SUCCESS ]
```

 ## Compile Output
The html manual and meta data code are automatically generated and the code is compiled as .o object files.

1. For easy review, SAI meata API can be viewed in html:

    firefox html/index.html

Or you can directly access a API (eg. attenuator):

    firefox html/group__SAIEXPERIMENTALOTAIATTENUATOR.html

  <img src="https://github.com/oplklum/Images/blob/master/otai-atten.png?raw=true">

2. New created files saimetadata.h and saimetadata.c includes OTAI definitions, such as:
``````
    const sai_api_extensions_t sai_metadata_sai_api_extensions_t_enum_values[] = {
        SAI_API_BMTOR,
        SAI_API_OTAI_ATTENUATOR,
        SAI_API_OTAI_OA,
        SAI_API_OTAI_OCM,
        SAI_API_OTAI_OSC,
        SAI_API_OTAI_OTDR,
        -1
    };

    const sai_object_type_extensions_t sai_metadata_sai_object_type_extensions_t_enum_values[] = {
        SAI_OBJECT_TYPE_TABLE_BITMAP_CLASSIFICATION_ENTRY,
        SAI_OBJECT_TYPE_TABLE_BITMAP_ROUTER_ENTRY,
        SAI_OBJECT_TYPE_TABLE_META_TUNNEL_ENTRY,
        SAI_OBJECT_TYPE_OTAI_ATTENUATOR,
        SAI_OBJECT_TYPE_OTAI_OA,
        SAI_OBJECT_TYPE_OTAI_OCM,
        SAI_OBJECT_TYPE_OTAI_OSC,
        SAI_OBJECT_TYPE_OTAI_OTDR,
        -1
    };

    const sai_enum_metadata_t sai_metadata_enum_sai_otai_attenuator_attr_t = {
        .name              = "sai_otai_attenuator_attr_t",
        .valuescount       = 8,
        .values            = (const int*)sai_metadata_sai_otai_attenuator_attr_t_enum_values,
        .valuesnames       = sai_metadata_sai_otai_attenuator_attr_t_enum_values_names,
        .valuesshortnames  = sai_metadata_sai_otai_attenuator_attr_t_enum_values_short_names,
        .containsflags     = false,
        .flagstype         = SAI_ENUM_FLAGS_TYPE_NONE,
        .ignorevalues      = NULL,
        .ignorevaluesnames = NULL,
        .objecttype        = (sai_object_type_t)SAI_OBJECT_TYPE_OTAI_ATTENUATOR,
    };

    const sai_otai_attenuator_stat_t sai_metadata_sai_otai_attenuator_stat_t_enum_values[] = {
        SAI_OTAI_ATTENUATOR_STAT_ACTUAL_ATTENUATION,
        SAI_OTAI_ATTENUATOR_STAT_OUTPUT_POWER_TOTAL,
        SAI_OTAI_ATTENUATOR_STAT_OPTICAL_RETURN_LOSS,
        -1
    };
    
``````
3. Object file ``saimetadata.o`` will be use to build ``libsairedis``.

## Note
- In this prototype, OTAI only contains OA, OCM, ATTENUATOR and OTDR. However, this mechanism is generic. To include other OTAI objects and APIs, simply add other OTAI header files into OTAI submodule.
- ``extension.py`` can be integrated into sonic-sairedis build process for otn platforms. For existing switch platforms, everything works as is, nothing changed.