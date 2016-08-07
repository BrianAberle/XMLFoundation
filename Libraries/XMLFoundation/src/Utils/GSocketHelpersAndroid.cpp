/* external/dhcpcd/ifaddrs.c
**
** Copyright 2011, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");.
** you may not use this file except in compliance with the License..
** You may obtain a copy of the License at.
**
**     http://www.apache.org/licenses/LICENSE-2.0.
**
** Unless required by applicable law or agreed to in writing, software.
** distributed under the License is distributed on an "AS IS" BASIS,.
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied..
** See the License for the specific language governing permissions and.
** limitations under the License.
*/

#include <arpa/inet.h>
#include <sys/socket.h>
//#include "ifaddrs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <netinet/ether.h>
#include <netdb.h>
#include <linux/if_packet.h>
#include <netinet/if_ether.h>
#include <linux/if_arp.h>



//#include <netutils/ifc.h>
/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *
 *     http://www.apache.org/licenses/LICENSE-2.0 
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */

#ifndef _NETUTILS_IFC_H_
#define _NETUTILS_IFC_H_

#include <sys/cdefs.h>
#include <arpa/inet.h>

__BEGIN_DECLS

extern int ifc_init(void);
extern void ifc_close(void);

extern int ifc_get_ifindex(const char *name, int *if_indexp);
extern int ifc_get_hwaddr(const char *name, void *ptr);

extern int ifc_up(const char *name);
extern int ifc_down(const char *name);

extern int ifc_enable(const char *ifname);
extern int ifc_disable(const char *ifname);

#define RESET_IPV4_ADDRESSES 0x01
#define RESET_IPV6_ADDRESSES 0x02
#define RESET_IGNORE_INTERFACE_ADDRESS 0x04
#define RESET_ALL_ADDRESSES  (RESET_IPV4_ADDRESSES | RESET_IPV6_ADDRESSES)
extern int ifc_reset_connections(const char *ifname, const int reset_mask);

extern int ifc_get_addr(const char *name, in_addr_t *addr);
extern int ifc_set_addr(const char *name, in_addr_t addr);
extern int ifc_add_address(const char *name, const char *address,
                           int prefixlen);
extern int ifc_del_address(const char *name, const char *address,
                           int prefixlen);
extern int ifc_set_prefixLength(const char *name, int prefixLength);
extern int ifc_set_hwaddr(const char *name, const void *ptr);
extern int ifc_clear_addresses(const char *name);

extern int ifc_create_default_route(const char *name, in_addr_t addr);
extern int ifc_remove_default_route(const char *ifname);
extern int ifc_get_info(const char *name, in_addr_t *addr, int *prefixLength,
                        unsigned *flags);

extern int ifc_configure(const char *ifname, in_addr_t address,
                         uint32_t prefixLength, in_addr_t gateway,
                         in_addr_t dns1, in_addr_t dns2);

extern in_addr_t prefixLengthToIpv4Netmask(int prefix_length);

__END_DECLS

#endif /* _NETUTILS_IFC_H_ */




// begin from ifc_utils.c -------------------------------------------------------------------------------------

#define SOCK_CLOEXEC	02000000

static int ifc_ctl_sock = -1;

static void ifc_init_ifr(const char *name, struct ifreq *ifr)
{
    memset(ifr, 0, sizeof(struct ifreq));
    strncpy(ifr->ifr_name, name, IFNAMSIZ);
    ifr->ifr_name[IFNAMSIZ - 1] = 0;
}

int ipv4NetmaskToPrefixLength(in_addr_t mask)
{
    int prefixLength = 0;
    uint32_t m = (uint32_t)ntohl(mask);
    while (m & 0x80000000) {
        prefixLength++;
        m = m << 1;
    }
    return prefixLength;
}

int ifc_get_info(const char *name, in_addr_t *addr, int *prefixLength, unsigned *flags)
{
    struct ifreq ifr;
    ifc_init_ifr(name, &ifr);

    if (addr != NULL) {
        if(ioctl(ifc_ctl_sock, SIOCGIFADDR, &ifr) < 0) {
            *addr = 0;
        } else {
            *addr = ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr;
        }
    }

    if (prefixLength != NULL) {
        if(ioctl(ifc_ctl_sock, SIOCGIFNETMASK, &ifr) < 0) {
            *prefixLength = 0;
        } else {
            *prefixLength = ipv4NetmaskToPrefixLength(
                    ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr);
        }
    }

    if (flags != NULL) {
        if(ioctl(ifc_ctl_sock, SIOCGIFFLAGS, &ifr) < 0) {
            *flags = 0;
        } else {
            *flags = ifr.ifr_flags;
        }
    }

    return 0;
}

int ifc_get_hwaddr(const char *name, void *ptr)
{
    int r;
    struct ifreq ifr;
    ifc_init_ifr(name, &ifr);

    r = ioctl(ifc_ctl_sock, SIOCGIFHWADDR, &ifr);
    if(r < 0) return -1;

    memcpy(ptr, &ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    return 0;
}


in_addr_t prefixLengthToIpv4Netmask(int prefix_length)
{
    in_addr_t mask = 0;

    // C99 (6.5.7): shifts of 32 bits have undefined results
    if (prefix_length <= 0 || prefix_length > 32) {
        return 0;
    }

    mask = ~mask << (32 - prefix_length);
    mask = htonl(mask);

    return mask;
}

int ifc_init(void)
{
    int ret;
    if (ifc_ctl_sock == -1) {
        ifc_ctl_sock = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
//        if (ifc_ctl_sock < 0) {
//            printerr("socket() failed: %s\n", strerror(errno));
//        }
    }

    ret = ifc_ctl_sock < 0 ? -1 : 0;
//    if (DBG) printerr("ifc_init_returning %d", ret);
    return ret;
}
void ifc_close(void)
{
//    if (DBG) printerr("ifc_close");
    if (ifc_ctl_sock != -1) {
        (void)close(ifc_ctl_sock);
        ifc_ctl_sock = -1;
    }
}
// end from ifc_utils.c -------------------------------------------------------------------------------------


struct ifaddrs *get_interface(const char *name, sa_family_t family)
{
    unsigned addr, flags;
    int masklen;
    struct ifaddrs *ifa;
    struct sockaddr_in *saddr = NULL;
    struct sockaddr_in *smask = NULL;
    struct sockaddr_ll *hwaddr = NULL;
    unsigned char hwbuf[ETH_ALEN];

    if (ifc_get_info(name, &addr, &masklen, &flags))
        return NULL;

    if ((family == AF_INET) && (addr == 0))
        return NULL;

    ifa = (struct ifaddrs *)malloc(sizeof(struct ifaddrs));
    if (!ifa)
        return NULL;
    memset(ifa, 0, sizeof(struct ifaddrs));

    ifa->ifa_name = (char *)malloc(strlen(name)+1);
    if (!ifa->ifa_name) {
        free(ifa);
        return NULL;
    }
    strcpy(ifa->ifa_name, name);
    ifa->ifa_flags = flags;

    if (family == AF_INET) {
        saddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        if (saddr) {
            saddr->sin_addr.s_addr = addr;
            saddr->sin_family = family;
        }
        ifa->ifa_addr = (struct sockaddr *)saddr;

        if (masklen != 0) {
            smask = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
            if (smask) {
                smask->sin_addr.s_addr = prefixLengthToIpv4Netmask(masklen);
                smask->sin_family = family;
            }
        }
        ifa->ifa_netmask = (struct sockaddr *)smask;
    } else if (family == AF_PACKET) {
        if (!ifc_get_hwaddr(name, hwbuf)) {
            hwaddr =(struct sockaddr_ll *) malloc(sizeof(struct sockaddr_ll));
            if (hwaddr) {
                memset(hwaddr, 0, sizeof(struct sockaddr_ll));
                hwaddr->sll_family = family;
                /* hwaddr->sll_protocol = ETHERTYPE_IP; */
                hwaddr->sll_hatype = ARPHRD_ETHER;
                hwaddr->sll_halen = ETH_ALEN;
                memcpy(hwaddr->sll_addr, hwbuf, ETH_ALEN);
            }
        }
        ifa->ifa_addr = (struct sockaddr *)hwaddr;
        ifa->ifa_netmask = (struct sockaddr *)smask;
    }
    return ifa;
}

int getifaddrs(struct ifaddrs **ifap)
{
    DIR *d;
    struct dirent *de;
    struct ifaddrs *ifa;
    struct ifaddrs *ifah = NULL;

    if (!ifap)
        return -1;
    *ifap = NULL;

    if (ifc_init())
       return -1;

    d = opendir("/sys/class/net");
    if (d == 0)
        return -1;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.')
            continue;
        ifa = get_interface(de->d_name, AF_INET);
        if (ifa != NULL) {
            ifa->ifa_next = ifah;
            ifah = ifa;
        }
        ifa = get_interface(de->d_name, AF_PACKET);
        if (ifa != NULL) {
            ifa->ifa_next = ifah;
            ifah = ifa;
        }
    }
    *ifap = ifah;
    closedir(d);
    ifc_close();
    return 0;
}

void freeifaddrs(struct ifaddrs *ifa)
{
    struct ifaddrs *ifp;

    while (ifa) {
        ifp = ifa;
        free(ifp->ifa_name);
        if (ifp->ifa_addr)
            free(ifp->ifa_addr);
        if (ifp->ifa_netmask)
            free(ifp->ifa_netmask);
        ifa = ifa->ifa_next;
        free(ifp);
    }
}
