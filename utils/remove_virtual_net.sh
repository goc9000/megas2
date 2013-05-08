#!/bin/bash

BASE_DIR=$( cd "$(dirname "$0")" ; pwd -P )

sudo kill `cat "$BASE_DIR/temp_dhcpd.pid"`
sudo rm "$BASE_DIR/temp_dhcpd.pid"

sudo ifconfig megasnet0 down
sudo tunctl -d megasnet0
