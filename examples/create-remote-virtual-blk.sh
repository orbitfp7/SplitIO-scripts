#! /bin/bash

vFE_MAC="fa:16:3e:f1:03:c6"
vFE_IP="10.0.0.208"

iohyp_create_blk_device.sh -i io-veth100  -p /dev/ram1 -w vdb -g $vFE_MAC -t $vFE_IP > /tmp/vrio
rc=$?
if [[ $rc == 0 ]]; then
    echo "Virtual block device created successfully"
else
    echo "Virtual block device creation failed"
fi
