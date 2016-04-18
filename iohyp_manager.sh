#!/bin/bash

HOST=$(hostname)
NIC_MAC_ADDRESS=''
UNDERLYING_INTERFACE_NAME=''
DEVICE_PATH=''

# find where this script is being run from (could be local, could be in the path)
fullpath=$(which $0)
script_path=${fullpath%/*}

function usage {
    echo "$0"
    echo "   -c                           Compile iohyp_manager.c"
    echo " "
    echo "   -i <ifname>                  interface name (VF)"
    echo "   -g <xx:xx:xx:xx:xx:xx>       Guest MAC address"
    echo "   -t <x.x.x.x>                 Guest IP address"
    echo "   -r                           Remove device/all"
    echo "   -p <path>                    Device path"
    echo "   -a <xx:xx:xx:xx:xx:xx>       NIC MAC address"
    echo "   -u <ifname>                  Macvtap underlaying inteface name"    
    echo "   -f                           Use TSO/GSO with vNIC"    
    echo "   -m                           Max block segments"
    echo " "
    echo "   -b                           Add block device"
    echo "   -n                           Add network device"
    echo "   -v <iocores list>            Set poll mode (prepend minus to remove)"
    echo "   -k <iocores list>            Set active iocores" 
    echo "   -s <blk/net>                 Sanity check"
    echo "   -h                           help"
    echo " "
    echo "   -d <mtap name>               Delete macvtap device"
    exit
}

# print the usage and exit in case no arguments specified.
if [ $# -eq 0 ]
then
   usage
fi

while getopts a:bcd:efg:hi:jk:l:mno:p:q:r:s:t:u:v:w:x:yz option
do
        case "${option}"
        in 
            c) cd $script_path/iohyp_manager
               make	
	       exit $?;;
            a) NIC_MAC_ADDRESS=$OPTARG;;
            u) UNDERLYING_INTERFACE_NAME=$OPTARG;;
            p) DEVICE_PATH=$OPTARG;;  
            n) if [ "${DEVICE_PATH:0:1}" = "m" ] ; then
		    sudo ip link delete dev $DEVICE_PATH type macvtap >& /dev/null
                    sudo ip link add link $UNDERLYING_INTERFACE_NAME name $DEVICE_PATH type macvtap
                    sudo ip link set $DEVICE_PATH address $NIC_MAC_ADDRESS
                    sleep 2
                    sudo ifconfig $DEVICE_PATH up
               fi;;
            d) sudo ip link delete dev $OPTARG type macvtap         
               exit 1;;
            h) usage
       esac
done

sudo $script_path/iohyp_manager/iohyp_manager "$@"
exit $?
