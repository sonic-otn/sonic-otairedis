#!/usr/bin/env bash

CMD_SYNCD=/usr/bin/syncd

PLATFORM_DIR=/usr/share/sonic/platform
PROFILE_FILE=/tmp/otai_test.profile

VARS_FILE=/usr/share/sonic/templates/swss_vars.j2
# Retrieve vars from sonic-cfggen
SYNCD_VARS=$(sonic-cfggen -d -y /etc/sonic/sonic_version.yml -t $VARS_FILE) || exit 1
SONIC_ASIC_TYPE=$(echo $SYNCD_VARS | jq -r '.asic_type')
ASIC_ID=$(echo $SYNCD_VARS | jq -r '.asic_id')
SLOT_ID=$((AISC_ID+1))
echo "OTAI_LINECARD_LOCATION=$SLOT_ID" > $PROFILE_FILE

CMD=$CMD_SYNCD
CMD_ARGS=

config_syncd_vs()
{
    echo "OTAI_VS_LINECARD_TYPE=OTAI_VS_LINECARD_TYPE_OTN" >> $PROFILE_FILE
    CMD_ARGS+=" -p $PROFILE_FILE"
}

config_syncd_accelink()
{
    CMD_ARGS+=" -p $PROFILE_FILE"
}

config_syncd()
{
    if [ "$SONIC_ASIC_TYPE" == "ot-vs" ]; then
        config_syncd_vs
    elif [ "$SONIC_ASIC_TYPE" == "ot-accelink" ]; then
        config_syncd_accelink
    else
        echo "Unknown ASIC type $SONIC_ASIC_TYPE"
        exit 1
    fi

    [ -r $PLATFORM_DIR/syncd.conf ] && . $PLATFORM_DIR/syncd.conf
}

