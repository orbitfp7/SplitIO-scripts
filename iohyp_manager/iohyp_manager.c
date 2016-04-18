#define _LARGEFILE64_SOURCE /* See feature_test_macros(7) */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <getopt.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>

/* Included from Linux Kernel source tree */
#include "generic_common.h"
#include "sdev.h"

#define trace(fmt, args...) \
    printf("[iohyp_manager] <%s>: "fmt"\n", __func__, ##args);

void print_ioctl_param(struct ioctl_param *param);
int fd;

int generic_open(void) {
    return open("/dev/iohyp", O_RDWR);
}

int generic_close(int fd) {    
    return close(fd);
}

static struct ioctl_param sdev_param = {
    .device_name = "sdev",
    .interface_name = "ethBE",
    .guest_mac_address = "\x00\x16\x32\x64\x80\xa2",

    .x.sdev.udelay = 5000,
    .x.sdev.nr_iov = 10,
    .cmd = VRIO_IOCTL_CREATE_SDEV,
};

static struct ioctl_param channel_param = {    
    .device_name = "any",
    .interface_name = "ethBE",
};

static struct ioctl_param iocore_param = {    
    .device_name = "any",
    .interface_name = "ethBE",
};

static struct ioctl_param poll_param = {    
    .device_name = "net",
    .interface_name = "ethBE",
};

static struct ioctl_param sanity_param = {    
    .device_name = "blk",
    .interface_name = "ethBE",
    .guest_mac_address = "\x00\x16\x32\x64\x80\xa2",

    .cmd = VRIO_IOCTL_SANITY_CHECK,
};

static struct ioctl_param create_blk = {
    .device_name = "blk",
    .interface_name = "ethBE",
    .guest_mac_address = "\x00\x16\x32\x64\x80\xa2",
    .x.create.device_path = "/dev/ram1",
    .x.create.config.vrio_blk.features = ((1 << VIRTIO_BLK_F_WCE)       |
                                          (1 << VIRTIO_BLK_F_SEG_MAX)),
    .x.create.config.vrio_blk.seg_max = 126,
    .cmd = VRIO_IOCTL_CREATE_BLK,
};

static struct ioctl_param remove_dev = {    
    .device_name = "blk",
    .interface_name = "ethBE",
    .guest_mac_address = "\x00\x16\x32\x64\x80\xa2",
    .x.remove.device_id = 0,

    .cmd = VRIO_IOCTL_REMOVE_DEV,
};

static struct ioctl_param create_net = {    
    .device_name = "net",
    .interface_name = "ethBE",
    .guest_mac_address = "\x00\x16\x32\x64\x80\xa2",
    .x.create.device_path = "vtap0",
    .x.create.config.vrio_net.features = ((1 << VIRTIO_NET_F_MAC)), 
    .x.create.config.vrio_net.mac = "\x00\x16\x32\x64\x81\x10",
    
    .cmd = VRIO_IOCTL_CREATE_NET,
};

int string2array(char *string, int arr[]) {
    char *delim = ",";
    int i = 0, num;

    string = strtok(string, delim);
    while (string) {        
        num = atoi(string);
        if (num == 0)
            return -1;

        arr[i++] = num;
        string = strtok(NULL, delim);
    }

    return i;
}

long get_bdev_capacity(char *path) {
    int blkfd;
    long len;

    blkfd = open(path, O_RDWR);
    if (blkfd < 0) {
        trace("failed to open %s", path);
        close(blkfd);
        return 0;
    }
    len = lseek64(blkfd, 0, SEEK_END);
    close(blkfd);

    trace("block device %s sectors count: %lu", path, len / 512);
    return len;
}

/**
 *  This subroutine processes the command line options 
 */
void process_options(int argc, char *argv[])
{
    int error = 0, res;
    int c, i;
    int mac_address[6];

    for (;;) {
        c = getopt(argc, argv, "w:l:o:q:x:ejk:m:v:a:d:i:g:p:bns:r:ht:u:fy:zq:");
        if (c == -1)
            break;

        switch (c) {            
            case 'w': {
                strncpy(create_blk.x.create.config.vrio_blk.device_name, optarg, 
                    sizeof(create_blk.x.create.config.vrio_blk.device_name));
                create_blk.x.create.config.vrio_blk.features |= (1 << VIRTIO_BLK_F_DEV_NAME);
            }

            case 'm': {
                res = sscanf(optarg, "%d", &i);
                if (i >= 0 && i <= 126)
                   create_blk.x.create.config.vrio_blk.seg_max = i;		
                break;
            }

           case 'i': {
                strncpy(create_blk.interface_name, optarg, sizeof(create_blk.interface_name));
                strncpy(create_net.interface_name, optarg, sizeof(create_net.interface_name));
                strncpy(remove_dev.interface_name, optarg, sizeof(remove_dev.interface_name));
                strncpy(sanity_param.interface_name, optarg, sizeof(sanity_param.interface_name));            
                strncpy(poll_param.interface_name, optarg, sizeof(poll_param.interface_name));            
                strncpy(iocore_param.interface_name, optarg, sizeof(iocore_param.interface_name));            
                strncpy(channel_param.interface_name, optarg, sizeof(channel_param.interface_name));                        
                strncpy(sdev_param.interface_name, optarg, sizeof(sdev_param.interface_name));
                break;
            }
    
            case 'g': {
                res = sscanf(optarg, "%x:%x:%x:%x:%x:%x", &mac_address[0], &mac_address[1], &mac_address[2],
                  &mac_address[3], &mac_address[4], &mac_address[5]);
                if (res == 6) {
                    for (i=0; i<6; i++) {
                        create_blk.guest_mac_address[i] = (char)mac_address[i];
                        create_net.guest_mac_address[i] = (char)mac_address[i];
                        remove_dev.guest_mac_address[i] = (char)mac_address[i];
                        sanity_param.guest_mac_address[i] = (char)mac_address[i];
                        sdev_param.guest_mac_address[i] = (char)mac_address[i];
                    }
                } else {
                    trace("MAC address malformed");
                }
                break;
            }

            case 'a': {
                res = sscanf(optarg, "%x:%x:%x:%x:%x:%x", &mac_address[0], &mac_address[1], &mac_address[2],
                  &mac_address[3], &mac_address[4], &mac_address[5]);
                if (res == 6) {
                    for (i=0; i<6; i++) {
                        create_net.x.create.config.vrio_net.mac[i] = (char)mac_address[i];
                    }
                } else {
                    trace("MAC address malformed");
                }
                break;
            }

            case 'p': {
                strncpy(create_blk.x.create.device_path, optarg, sizeof(create_blk.x.create.device_path));            
                strncpy(create_net.x.create.device_path, optarg, sizeof(create_net.x.create.device_path));            
                strncpy(remove_dev.x.create.device_path, optarg, sizeof(remove_dev.x.create.device_path));            
                strncpy(sanity_param.x.create.device_path, optarg, sizeof(sanity_param.x.create.device_path));                        
                break;
            }

            case 'k': {
                trace("setting active iocores for interface %s (%s)", poll_param.interface_name, optarg);
                res = string2array(optarg, iocore_param.x.iocore.iocores);
                if (res == -1) {
                    trace("Polling mode malformed");
                    break;
                }
                iocore_param.x.iocore.nr_iocores = res;
                print_ioctl_param(&iocore_param);
                res = ioctl(fd, GENERIC_IOCTL_IOCORE, (ulong)&iocore_param);
                if (res) {
                    trace("ioctl failed %d", res);        
                }
                break;
            }

            case 'v': {
                trace("setting poll mode for interface %s (%s)", poll_param.interface_name, optarg);
                res = string2array(optarg, poll_param.x.poll.iocores);
                if (res == -1) {
                    trace("Polling mode malformed");
                    break;
                }
                poll_param.x.poll.nr_iocores = res;
                print_ioctl_param(&poll_param);
                res = ioctl(fd, GENERIC_IOCTL_POLL, (ulong)&poll_param);
                if (res) {
                    trace("ioctl failed %d", res);        
                }
                break;
            }

            case 'z': {
                trace("creating channel on interface %s", channel_param.interface_name);
                print_ioctl_param(&channel_param);
                res = ioctl(fd, GENERIC_IOCTL_CHANNEL, (ulong)&channel_param);
                if (res) {
                    trace("ioctl failed %d", res);        
                }
                break;
            }

            case 'f': {
                create_net.x.create.config.vrio_net.features = ((1 << VIRTIO_NET_F_MAC)       |
                                                              (1 << VIRTIO_NET_F_CSUM)      |
                                                              (1 << VIRTIO_NET_F_GSO)       |
                                                              (1 << VIRTIO_NET_F_HOST_TSO4) |
                                                              (1 << VIRTIO_NET_F_HOST_TSO6) |
                                                              (1 << VIRTIO_NET_F_HOST_ECN)  |
                                                              (1 << VIRTIO_NET_F_HOST_UFO)  |                                                                                                
                                                              (1 << VIRTIO_NET_F_GUEST_TSO4) |
                                                              (1 << VIRTIO_NET_F_GUEST_TSO6) |
                                                              (1 << VIRTIO_NET_F_GUEST_ECN)
                                                              );
                break;                  
            }

            case 'n': {
                trace("adding net device %s", create_net.x.create.device_path);
                print_ioctl_param(&create_net);
                res = ioctl(fd, GENERIC_IOCTL_CREATE, (ulong)&create_net);
                if (res) {
                    trace("ioctl failed %d", res);        
                }
                break;
            }

            case 'b': {
                trace("adding blk device %s", create_blk.x.create.device_path);
                create_blk.x.create.config.vrio_blk.capacity = get_bdev_capacity(create_blk.x.create.device_path);
                print_ioctl_param(&create_blk);
                res = ioctl(fd, GENERIC_IOCTL_CREATE, (ulong)&create_blk);
                if (res) {
                    trace("ioctl failed %d", res);        
                }
                break;
            }

            case 'e': {
                trace("adding sdev device");
                sdev_param.cmd = VRIO_IOCTL_CREATE_SDEV;
                print_ioctl_param(&sdev_param);
                res = ioctl(fd, GENERIC_IOCTL_CREATE, (ulong)&sdev_param);
                if (res) {
                    trace("ioctl failed %d", res);        
                }
                break;
            }

            case 'x': {
                res = sscanf(optarg, "%d", &i);
                sdev_param.x.sdev.size = i;            
                trace("set size: %d", i);
                break;
            }

            case 'o': {
                res = sscanf(optarg, "%d", &i);
                sdev_param.x.sdev.operation = i;            
                trace("set operation: %d", i);
                break;
            }

            case 'q': {
                res = sscanf(optarg, "%d", &i);
                sdev_param.x.sdev.udelay = i;            
                trace("set udelay: %d", i);
                break;
            }

            case 'l': {
                res = sscanf(optarg, "%d", &i);
                sdev_param.x.sdev.duration = i;            
                trace("set duration: %d", i);
                break;
            }

            case 'y': {
                res = sscanf(optarg, "%d", &i);
                sdev_param.x.sdev.nr_iov = i;            
                trace("set nr_iov: %d", i);
                break;
            }

            case 'j': {
                trace("request for sdev device");
                sdev_param.cmd = VRIO_IOCTL_REQUEST_SDEV;
                sdev_param.x.sdev.flags = SDEV_FLAGS_WRITE_ZCOPY | SDEV_FLAGS_READ_ZCOPY | SDEV_FLAGS_ACK_ON_WRITE | SDEV_FLAGS_CHECKSUM;
                print_ioctl_param(&sdev_param);
                res = ioctl(fd, GENERIC_IOCTL_GENERIC, (ulong)&sdev_param);
                if (res) {
                    trace("ioctl failed %d", res);        
                }
                break;
            }

            case 'r': {            
                strncpy(remove_dev.device_name, optarg, sizeof(remove_dev.device_name));
                res = ioctl(fd, GENERIC_IOCTL_REMOVE, (ulong)&remove_dev);
                if (res) {
                    trace("ioctl failed %d", res);        
                }
                break;
            }

            case 's': {            
                trace("sanity check");
                strncpy(sanity_param.device_name, optarg, sizeof(sanity_param.device_name));
                res = ioctl(fd, GENERIC_SANITY_CHECK, (ulong)&sanity_param);
                if (res) {
                  trace("ioctl failed %d", res);        
                }
                break;
            }

            case 'u':
                break;

            case 't': {
                int ip_addr = inet_addr(optarg);
                if (ip_addr != -1) { // INADDR_NONE) {
                    create_blk.guest_ip_address   = ip_addr;
                    create_net.guest_ip_address   = ip_addr;
                    remove_dev.guest_ip_address   = ip_addr;
                    sanity_param.guest_ip_address = ip_addr;
                    sdev_param.guest_ip_address   = ip_addr;
                } else {
                    trace("IP address malformed");
                }
                break;
            }

        default: 
            error = 1;
            break;
        }
    }
    
    if (error || res) {
        exit(EXIT_FAILURE);
    }
}

void print_ioctl_param(struct ioctl_param *param) {
    trace("device_name: %s", param->device_name);
    trace("interface_name: %s", param->interface_name);
    trace("guest_mac_address: %02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)param->guest_mac_address[0], 
                                                              (unsigned char)param->guest_mac_address[1], 
                                                              (unsigned char)param->guest_mac_address[2], 
                                                              (unsigned char)param->guest_mac_address[3],
                                                              (unsigned char)param->guest_mac_address[4], 
                                                              (unsigned char)param->guest_mac_address[5]);
    trace("x.create.device_path: %s", param->x.create.device_path);

    if (strcmp(param->device_name, "blk") == 0) {
        trace("x.create.config.vrio_blk.seg_max: %d", param->x.create.config.vrio_blk.seg_max);
        trace("x.create.config.vrio_blk.features: %lX", param->x.create.config.vrio_blk.features);
    } else if (strcmp(param->device_name, "net") == 0) {
        trace("x.create.config.vrio_net.features: %lX", param->x.create.config.vrio_net.features);    
    }
}

int main(int argc, char** argv) {	        
    int i, nr, arr[32];
    trace("iohyp manager");

    fd = generic_open();
    if (fd < 0) {
        trace("There was an error opening iohyp");
        exit(EXIT_FAILURE);
    }

    process_options(argc, argv);		
    
    if (generic_close(fd)) {
        trace("There was an error closing iohyp");
        exit(EXIT_FAILURE);
    }
    
    exit(EXIT_SUCCESS);
}