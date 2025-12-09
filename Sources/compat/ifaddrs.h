#ifndef IFADDRS_H_
#define IFADDRS_H_

/* Stub de compatibilidade para <ifaddrs.h> em ambiente bare-metal.
 * O open62541 pode usar getifaddrs/freeifaddrs para descobrir
 * interfaces de rede em sistemas POSIX. Aqui fornecemos tipos e
 * funções mínimas para permitir a compilação, mas sem funcionalidade
 * real.
 */

struct sockaddr; /* forward declaration suficiente para ponteiros */

struct ifaddrs {
    struct ifaddrs *ifa_next;
    char           *ifa_name;
    unsigned int    ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
    union {
        struct sockaddr *ifu_broadaddr;
        struct sockaddr *ifu_dstaddr;
    } ifa_ifu;
    void           *ifa_data;
};

#define ifa_broadaddr ifa_ifu.ifu_broadaddr
#define ifa_dstaddr   ifa_ifu.ifu_dstaddr

static inline int getifaddrs(struct ifaddrs **ifap)
{
    (void)ifap;
    /* Sem suporte a enumeração de interfaces neste ambiente. */
    return -1;
}

static inline void freeifaddrs(struct ifaddrs *ifa)
{
    (void)ifa;
}

#endif /* IFADDRS_H_ */
