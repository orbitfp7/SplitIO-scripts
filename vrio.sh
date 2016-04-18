#!/bin/bash

ethFE="vFE"
ethBE="vBE"
vMTU=8100
use_BIO=0
g_cpu_affinity=-1 #4
h_cpu_affinity=-1 #3
outstanding_reqs=128
VHOST_NET_WEIGHT=0x80000 
work_queue_size=2048
l2_packet_list_size=4096
large_packet_list_size=4096
zcopytx=0

mode='n'

function trace {
	echo -e $@
}

function etrace {
	trace "\e[31m"$@"\e[0m"
}

function usage {
    echo "$0"
    echo "   -v vMTU                      vMTU"
    echo "   -f <eth name>                Front-End eth name"
    echo "   -b <eth name>                Back-End eth name"
    echo "   -u <bio>                     use BIO"
    echo "   -z <zcopy>                   Use zero copy in vhost"    
    echo "   -o <reqs>                    Block max outstanding requests"
    echo "   -w <weight>                  Net vhost weight"
    echo "   -q <size>                    Generic work queue size"
    echo "   -i <cpuID>                   IOHyp CPU affinity (starts from 1)"
    echo "   -g <cpuID>                   Guest CPU affinity (starts from 1)"
    echo "   -p <list size>               l2packet list size (h/g 4096/65536)"
    echo "   -l <list size>               large_packet list size (h/g 2048/32768)"
    echo "   -c <ueth, veth>              Create macvtap (veth) device on top of ueth"
    echo "   -s                           Setup enviroment"
    echo " "
    echo "   -m <h/g/b>                   Mode: Host/Guest/Both"	
    echo "   -r                           Remove all vRIO modules"
    echo "   -h                           help"
    exit
}

function remove_mtap {
    local mtap_list=$(ifconfig -a | grep mtap | cut -d' ' -f1)
    
    trace "Removing mtap $mtap_list..."
    for mtap in $mtap_list; do
        trace "removing $mtap"
        sudo ip link delete dev $mtap type macvtap
    done
}

function remove_modules {
	trace "Removing any loaded modules..." 

	sudo rmmod vrio_hblk.ko >& /dev/null
	sudo rmmod vrio_gblk.ko >& /dev/null

	sudo rmmod vrio_hnet.ko >& /dev/null
	sudo rmmod vrio_gnet.ko >& /dev/null

	sudo rmmod vrio_gen.ko >& /dev/null
	sudo rmmod vrio_eth.ko >& /dev/null
}

function remove_all_modules {
	remove_modules
	
	trace "Removing macvtap & macvlan..."
	sudo rmmod macvtap.ko >& /dev/null
	sudo rmmod macvlan.ko >& /dev/null
}

function unload_vrio {
    remove_mtap
    remove_all_modules
}

function set_value {
	local path=$1
	local val=$2
	sudo sh -c "echo $val > $path"
}

function disable_multiqueue {
	local eth=$1

        sudo ethtool -L $eth tx 1 rx 1
}

function setup_enviroment {
	trace "Setting up enviroment..."
	local SK_MEM_MAX=3407872

	set_value "/proc/sys/net/core/wmem_max" $SK_MEM_MAX
	set_value "/proc/sys/net/core/rmem_max" $SK_MEM_MAX

	set_value "/proc/sys/net/core/wmem_default" $SK_MEM_MAX
	set_value "/proc/sys/net/core/rmem_default" $SK_MEM_MAX


	if [ $mode = "g" ] ; then
		disable_multiqueue $ethFE
	fi

	if [ $mode = "h" ] ; then
		disable_multiqueue $ethBE
	fi
}

# print the usage and exit in case no arguments specified.
if [ $# -eq 0 ]
then
   usage
fi

while getopts e:u:f:b:v:o:w:q:g:p:l:i:m:z:rRc:sh option
do
        case "${option}"
        in 
            u) use_BIO=$OPTARG;;
            z) zcopytx=$OPTARG;;
            f) ethFE=$OPTARG;;
            b) ethBE=$OPTARG;;
            v) vMTU=$OPTARG;;
            o) outstanding_reqs=$OPTARG;;
            w) VHOST_NET_WEIGHT=$OPTARG;;
            q) work_queue_size=$OPTARG;;
            i) h_cpu_affinity=$OPTARG;;
            g) g_cpu_affinity=$OPTARG;;
            p) l2_packet_list_size=$OPTARG;;
            l) large_packet_list_size=$OPTARG;;
            m) mode=$OPTARG;;
            r) remove_mtap
               remove_modules
               exit;;
            R) remove_mtap
               remove_all_modules
               exit;;
            c) pair=$OPTARG
               array=(${pair//,/ })
               udev=${array[0]}
               vdev=${array[1]}
               sudo ifconfig $udev txqueuelen 5000
               sudo ip link add link $udev name $vdev type macvtap   
               sudo ifconfig $vdev txqueuelen 5000 up
               exit;;	
            s) setup_enviroment
               exit;;	   
            h) usage;;
           \?) usage;;
       esac
done

setup_enviroment

remove_modules
trace "Loading vRIO modules..."

trace "Loading transportation layer with vMTU: "$vMTU", l2packet_list_size: "$l2_packet_list_size" and large_packet_list_size: "$large_packet_list_size

sudo modprobe vrio_eth.ko l2_packet_size=$vMTU \
                      l2_packet_list_size=$l2_packet_list_size \
                      large_packet_list_size=$large_packet_list_size

if [ $mode = "h" ] ; then
	trace "Loading generic layer (host) with VF: "$ethBE", affinity: "$h_cpu_affinity" and queue size: "$work_queue_size
	sudo modprobe vrio_gen.ko h_eth_name=$ethBE h_cpu_affinity=$h_cpu_affinity \
						  unit_test=0 \
					      work_queue_size=$work_queue_size
fi

if [ $mode = "g" ] ; then
	trace "Loading generic layer (guest) with VF: "$ethFE", affinity: "$g_cpu_affinity" and queue size: "$work_queue_size
	sudo modprobe vrio_gen.ko g_eth_name=$ethFE g_cpu_affinity=$g_cpu_affinity \
				      	  unit_test=0 \
					      work_queue_size=$work_queue_size
fi

if [ $mode = "b" ] ; then
	trace "Loading generic layer (both) with hVF: "$ethBE", gVF: "$ethFE", h_affinity: "$h_cpu_affinity", g_affinity: "$g_cpu_affinity" and queue size: "$work_queue_size
	sudo modprobe vrio_gen.ko h_eth_name=$ethBE h_cpu_affinity=$h_cpu_affinity \
						  g_eth_name=$ethFE g_cpu_affinity=$g_cpu_affinity \
  					      unit_test=0 \
					      work_queue_size=$work_queue_size
fi

rc=$?
if [[ $rc != 0 ]] ; then
    etrace "Failed to load generic layer, done."
    exit $rc
fi

if [ $mode = "h" ] || [ $mode = "b" ] ; then
	trace "Loading host blk and net layers with VHOST_NET_WEIGHT: "$VHOST_NET_WEIGHT", num_outstanding_reqs: "$outstanding_reqs" and zcopytx: "$zcopytx
	sudo modprobe vrio_hnet.ko VHOST_NET_WEIGHT=$VHOST_NET_WEIGHT zcopytx=$zcopytx
	sudo modprobe vrio_hblk.ko num_outstanding_reqs=$outstanding_reqs	
fi

if [ $mode = "g" ] || [ $mode = "b" ] ; then
	trace "Loading guest blk and net with use_bio: "$use_BIO" and num_outstanding_reqs: "$outstanding_reqs
	sudo modprobe vrio_gnet.ko
	sudo modprobe vrio_gblk.ko use_bio=$use_BIO	num_outstanding_reqs=$outstanding_reqs
fi
