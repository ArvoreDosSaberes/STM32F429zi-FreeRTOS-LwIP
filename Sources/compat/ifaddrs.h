/*
 * Header de compatibilidade POSIX para ifaddrs.h em ambiente bare-metal.
 * O open62541 pode usar esta API para enumerar interfaces de rede.
 * Aqui fornecemos apenas a estrutura e protótipos mínimos para
 * satisfazer o compilador, sem implementação real.
 */

#ifndef COMPAT_IFADDRS_H
#define COMPAT_IFADDRS_H

#include <stdint.h>
#include <stddef.h>
#include <net/if.h>
#include <netinet/in.h>

struct ifaddrs {
    struct ifaddrs  *ifa_next;
    char            *ifa_name;
    unsigned int     ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
    struct sockaddr *ifa_ifu;
    void            *ifa_data;
};

#ifdef __cplusplus
extern "C" {
#endif

static inline int getifaddrs(struct ifaddrs **ifap)
{
    (void)ifap;
    /* Nenhuma interface é retornada em ambiente bare-metal. */
    return 0;
}

static inline void freeifaddrs(struct ifaddrs *ifa)
{
    (void)ifa;
}

#ifdef __cplusplus
}
#endif

#endif /* COMPAT_IFADDRS_H */
