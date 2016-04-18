#! /bin/bash

vMTU=8100
INTERNAL_IF="io-veth100"

vrio.sh -r > /dev/null
vrio.sh -v $vMTU -f $INTERNAL_IF -g 1 -u 1 -m g &> /tmp/vrio
