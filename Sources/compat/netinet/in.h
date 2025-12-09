#ifndef __IN_H__
#define __IN_H__

#include "lwip/inet.h"

/* Garantir que macros POSIX de sockets (read/write/close/fcntl) nao sejam
 * habilitadas pelo LwIP neste TU, pois conflitam com o campo
 * vn->value.dataSource.read usado pelo open62541. */
#ifdef LWIP_POSIX_SOCKETS_IO_NAMES
#undef LWIP_POSIX_SOCKETS_IO_NAMES
#endif
#define LWIP_POSIX_SOCKETS_IO_NAMES 0

#include "lwip/sockets.h"

/* Se algum header anterior tiver definido read como macro, neutraliza aqui
 * para permitir o uso de campos/funcoes chamadas read no open62541. */
#ifdef read
#undef read
#endif

#include "lwip/igmp.h"
#include <stdint.h>

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

/* Stub mínimo da struct ip_mreqn usada em APIs de multicast em sistemas
 * Linux. O open62541 utiliza esse tipo para configurar multicast IPv4.
 * Aqui fornecemos um layout compatível suficiente para compilar, sem
 * semântica real de interface/index. */
struct ip_mreqn {
    struct in_addr imr_multiaddr; /* IP multicast address of group */
    struct in_addr imr_address;   /* local IP address of interface */
    int            imr_ifindex;   /* interface index */
};
#endif

/* Stubs mínimos de IPv6 para permitir que código gerado pelo open62541
 * compile em ambiente LwIP IPv4-only. Não habilita suporte real a IPv6. */

#ifndef LWIP_IPV6
#define LWIP_IPV6 0
#endif

#if !LWIP_IPV6

#ifndef AF_INET6
#define AF_INET6 10
#endif

#ifndef IPPROTO_IPV6
#define IPPROTO_IPV6 41
#endif

#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 26
#endif

#ifndef IPV6_JOIN_GROUP
#define IPV6_JOIN_GROUP 12
#endif

#ifndef IPV6_MULTICAST_IF
#define IPV6_MULTICAST_IF 17
#endif

/* Macros adicionais usadas por código IPv6 do open62541 em caminhos de
 * logging/multicast. Valores são placeholders suficientes para este
 * firmware (sem suporte real a IPv6). */
#ifndef IPV6_MULTICAST_LOOP
#define IPV6_MULTICAST_LOOP 0
#endif

#ifndef IPV6_MULTICAST_HOPS
#define IPV6_MULTICAST_HOPS 0
#endif

#ifndef IP_MULTICAST_LOOP
#define IP_MULTICAST_LOOP 0
#endif

/* Estruturas mínimas de socket IPv6 usadas pelo open62541 quando
 * LWIP_IPV6 está desabilitado. Não habilitam suporte real a IPv6. */
struct sockaddr_in6 {
    uint8_t         sin6_len;
    uint8_t         sin6_family;
    uint16_t        sin6_port;
    uint32_t        sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t        sin6_scope_id;
};

struct ipv6_mreq {
    struct in6_addr ipv6mr_multiaddr;
    unsigned int    ipv6mr_interface;
};

#endif /* !LWIP_IPV6 */

#endif
