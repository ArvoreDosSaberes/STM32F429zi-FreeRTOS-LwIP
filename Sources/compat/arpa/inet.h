#ifndef __ARPA_H__
#define __ARPA_H__

/* Stub de compatibilidade para <arpa/inet.h> em ambiente LwIP. Reexporta
 * a API de inet do LwIP e garante que as definições de netinet/in.h de
 * compat (IPv6 stub) também estejam disponíveis. */

#include "lwip/inet.h"
#include "netinet/in.h"

#endif /* __ARPA_H__ */
