#ifndef COMPAT_NETINET_IN_H
#define COMPAT_NETINET_IN_H

#include <lwip/inet.h>

/* Complementos mínimos quando o IPv6 está desabilitado na LwIP */
#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6 41
#endif

#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif

#ifndef IPV6_JOIN_GROUP
#define IPV6_JOIN_GROUP 20
#endif

#ifndef IPV6_MULTICAST_IF
#define IPV6_MULTICAST_IF 17
#endif

#ifndef IPV6_MULTICAST_HOPS
#define IPV6_MULTICAST_HOPS 18
#endif

#ifndef IPV6_MULTICAST_LOOP
#define IPV6_MULTICAST_LOOP 19
#endif

#ifndef IP_MULTICAST_IF
#define IP_MULTICAST_IF 32
#endif
#ifndef IP_MULTICAST_TTL
#define IP_MULTICAST_TTL 33
#endif
#ifndef IP_MULTICAST_LOOP
#define IP_MULTICAST_LOOP 34
#endif
#ifndef IP_ADD_MEMBERSHIP
#define IP_ADD_MEMBERSHIP 35
#endif

/* Estruturas básicas usadas pelas chamadas de multicast */
#ifndef HAVE_IPV6_MREQ
struct ipv6_mreq {
    struct in6_addr ipv6mr_multiaddr;
    unsigned int    ipv6mr_interface;
};
#endif

#ifndef HAVE_IP_MREQN
struct ip_mreqn {
    struct in_addr imr_multiaddr;
    struct in_addr imr_address;
    int            imr_ifindex;
};
#endif

#ifndef SOCKADDR_IN6_DEFINED
struct sockaddr_in6 {
    uint16_t       sin6_family;
    uint16_t       sin6_port;
    uint32_t       sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t       sin6_scope_id;
};
#endif

#endif /* COMPAT_NETINET_IN_H */
