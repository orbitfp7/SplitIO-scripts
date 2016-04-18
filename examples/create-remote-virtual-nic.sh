#! /bin/bash

EXTERNAL_IF=enp4s0
INTERNAL_IF=io-veth100

TAP="mtap1"

NEW_NIC_MAC=20:20:20:04:81:01
FE_IP="10.0.0.202"
FE_MAC="fa:16:3e:33:6b:64"
iohyp_create_nic_device.sh -i $INTERNAL_IF -g $FE_MAC -t $FE_IP -u $EXTERNAL_IF -a $NEW_NIC_MAC -p $TAP &> /tmp/vrio
rc=$?
if [[ $rc == 0 ]]; then
    echo "Virtual network device created successfully"
else
    echo "Virtual network device creation failed"
fi
