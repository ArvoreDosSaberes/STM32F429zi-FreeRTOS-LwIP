#ifndef __IN_H__
#define __IN_H__

#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/igmp.h"

/* Provide multicast-related definitions when lwIP options are disabled */
#ifndef IP_MULTICAST_IF
#define IP_MULTICAST_IF 6
#endif

#ifndef IP_MULTICAST_TTL
#define IP_MULTICAST_TTL 5
#endif

#ifndef IP_ADD_MEMBERSHIP
#define IP_ADD_MEMBERSHIP 3
#endif

#ifndef IP_DROP_MEMBERSHIP
#define IP_DROP_MEMBERSHIP 4
#endif

#ifndef LWIP_IGMP
#define LWIP_IGMP 0
#endif

#if !LWIP_IGMP
struct ip_mreq {
    struct in_addr imr_multiaddr; /* IP multicast address of group */
    struct in_addr imr_interface; /* local IP address of interface */
};
#endif

#endif
