#!/bin/bash

BASE_DIR=$( cd "$(dirname "$0")" ; pwd -P )
FOR_UID=`id -u`

sudo tunctl -t megasnet0 -u ${FOR_UID}

sudo ifconfig megasnet0 192.168.222.1 up

touch "$BASE_DIR/temp_dhcpd.leases"
sudo dhcpd -4 -cf "$BASE_DIR/sample_dhcpd.conf" -lf "$BASE_DIR/temp_dhcpd.leases" -pf "$BASE_DIR/temp_dhcpd.pid" megasnet0
