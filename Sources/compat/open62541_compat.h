#ifndef OPEN62541_COMPAT_H
#define OPEN62541_COMPAT_H

#include <stdint.h>
#include <stddef.h>

/* Tipos POSIX básicos (mode_t, uid_t, gid_t, pid_t, useconds_t, etc.)
 * virão das headers padrão (<sys/types.h>) da newlib. */

#include <sys/types.h>

/* Alguns headers da newlib (sys/time.h) usam u_int e sbintime_t sem
 * garantir que estejam definidos. Fornecemos typedefs mínimos aqui
 * para satisfazer essas dependências em ambiente bare-metal. */
#ifndef u_int
typedef unsigned int u_int;
#endif

#ifndef sbintime_t
typedef long long sbintime_t;
#endif

/* Sempre puxe primeiro os headers de compat, que já estão no include path
 * do projeto (Sources/compat). */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>

/* Garantir que constantes básicas de socket existam, caso os headers da
 * toolchain não as definam (ambiente bare-metal). */
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

#ifndef AF_UNSPEC
#define AF_UNSPEC 0
#endif

#ifndef SOCK_STREAM
#define SOCK_STREAM 1
#endif

#ifndef SOCK_DGRAM
#define SOCK_DGRAM 2
#endif

#ifndef SOL_SOCKET
#define SOL_SOCKET 1
#endif

#ifndef SO_REUSEADDR
#define SO_REUSEADDR 2
#endif

#ifndef IPPROTO_UDP
#define IPPROTO_UDP 17
#endif

/* Opções adicionais usadas pelo open62541 em código POSIX, mapeadas
 * aqui para valores simbólicos qualquer, apenas para satisfazer a
 * compilação e permitir tratamento simplificado no firmware. */

#ifndef SO_REUSEPORT
#define SO_REUSEPORT 15
#endif

#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif

#ifndef TCP_NODELAY
#define TCP_NODELAY 1
#endif

#ifndef SO_ERROR
#define SO_ERROR 4
#endif

/* clock_gettime e CLOCK_MONOTONIC_RAW podem não existir em bare-metal.
 * Fornecemos um stub básico baseado em um contador estático para
 * satisfazer o open62541. Idealmente, isso deve ser ligado a um
 * timer de hardware ou ao tick do FreeRTOS. */

#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW 4
#endif

static inline int compat_clock_gettime(clockid_t id, struct timespec *ts)
{
    (void)id;
    static unsigned long counter = 0;
    counter++;
    ts->tv_sec = 0;
    ts->tv_nsec = (long)(counter * 1000000UL); /* ~1ms fictício */
    return 0;
}

/* Redirecionar possíveis chamadas ao clock_gettime para o stub acima,
 * se a libc não fornecer uma implementação. */
#ifndef HAVE_CLOCK_GETTIME
#define clock_gettime compat_clock_gettime
#endif

#endif /* OPEN62541_COMPAT_H */
