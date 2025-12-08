#ifndef COMPAT_SYS_SOCKET_H
#define COMPAT_SYS_SOCKET_H

/*
 * Use a API de sockets da LwIP, que já fornece as definições POSIX
 * necessárias (AF_*, SOCK_*, SOL_SOCKET, TCP_NODELAY, etc.).
 */
#include <lwip/sockets.h>

#endif /* COMPAT_SYS_SOCKET_H */
