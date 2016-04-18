#! /bin/bash

iohyp_remove_net_device.sh -p mtap1 > /tmp/vrio
rc=$?
if [[ $rc == 0 ]]; then
    echo "Virtual network device successfully destroyed"
else
    echo "Virtual network device destruction failed"
fi
