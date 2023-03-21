#!/usr/bin/env bash
#
# Script to start syncd using supervisord
#

# Source the file that holds common code for systemd and supervisord
. /usr/bin/syncd_init_common.sh

config_syncd

sleep 12

exec ${CMD} ${CMD_ARGS}

