/* Garantir que a LwIP não redefina errno/valores POSIX */
#ifndef LWIP_PROVIDE_ERRNO
#define LWIP_PROVIDE_ERRNO 0
#endif
#ifndef LWIP_ERRNO_STDINCLUDE
#define LWIP_ERRNO_STDINCLUDE 1
#endif

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

int getnameinfo(const struct sockaddr *sa, socklen_t salen,
                char *host, socklen_t hostlen,
                char *serv, socklen_t servlen,
                int flags) {
    (void)sa;
    (void)salen;
    (void)flags;
    if(host && hostlen)
        host[0] = '\0';
    if(serv && servlen)
        serv[0] = '\0';
    return EAI_FAIL;
}

int gethostname(char *name, size_t len) {
    if(name && len) {
        name[0] = '\0';
        return 0;
    }
    errno = ENOSYS;
    return -1;
}

unsigned int if_nametoindex(const char *ifname) {
    (void)ifname;
    return 0;
}

int pipe(int fds[2]) {
    (void)fds;
    errno = ENOSYS;
    return -1;
}
