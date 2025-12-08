#ifndef COMPAT_NETDB_H
#define COMPAT_NETDB_H

#ifdef LWIP_PROVIDE_ERRNO
#undef LWIP_PROVIDE_ERRNO
#endif
#define LWIP_PROVIDE_ERRNO 0
#define LWIP_ERRNO_STDINCLUDE 1

#include <lwip/netdb.h>

#ifndef NI_MAXHOST
#define NI_MAXHOST 64
#endif

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 0x0001
#endif

/* LwIP pode não expor gai_strerror; fornecemos stub para logs */
static inline const char *gai_strerror(int errcode)
{
    (void)errcode;
    return "gai_error";
}

#endif /* COMPAT_NETDB_H */
