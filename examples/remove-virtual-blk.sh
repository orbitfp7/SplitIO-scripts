#! /bin/bash

iohyp_remove_blk_device.sh -p /dev/ram1 > /tmp/vrio
rc=$?
if [[ $rc == 0 ]]; then
    echo "Virtual block device successfully destroyed"
else
    echo "Virtual block device destruction failed"
fi
