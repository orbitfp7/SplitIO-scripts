#! /bin/bash

vMTU=8100

vrio.sh -r > /dev/null

# disable multiqueue
sudo ethtool -L enp4s0 rx 1 &> /tmp/null
sudo ethtool -L enp4s0d1 rx 1 &> /tmp/null

vrio.sh -v $vMTU -b "" -i "-1" -z 1 -o 256 -m h &> /tmp/vrio
