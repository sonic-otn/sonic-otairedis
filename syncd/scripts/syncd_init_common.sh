#!/usr/bin/env bash

CMD_SYNCD=/usr/bin/syncd

# dsserve: domain socket server for stdio
CMD_DSSERVE=/usr/bin/dsserve
CMD_DSSERVE_ARGS="$CMD_SYNCD --diag"

ENABLE_SAITHRIFT=0

PLATFORM_DIR=/usr/share/sonic/platform
PROFILE_FILE=/tmp/otai_test.profile

VARS_FILE=/usr/share/sonic/templates/swss_vars.j2
# Retrieve vars from sonic-cfggen
SYNCD_VARS=$(sonic-cfggen -d -y /etc/sonic/sonic_version.yml -t $VARS_FILE) || exit 1
SONIC_ASIC_TYPE=$(echo $SYNCD_VARS | jq -r '.asic_type')
ASIC_ID=$(echo $SYNCD_VARS | jq -r '.asic_id')
SLOT_ID=$((AISC_ID+1))
echo "OTAI_LINECARD_LOCATION=$SLOT_ID" > $PROFILE_FILE

if [ -x $CMD_DSSERVE ]; then
    CMD=$CMD_DSSERVE
    CMD_ARGS=$CMD_DSSERVE_ARGS
else
    CMD=$CMD_SYNCD
    CMD_ARGS=
fi

# Set synchronous mode if it is enabled in CONFIG_DB
SYNC_MODE=$(echo $SYNCD_VARS | jq -r '.synchronous_mode')
if [ "$SYNC_MODE" == "enable" ]; then
    CMD_ARGS+=" -s"
fi

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

