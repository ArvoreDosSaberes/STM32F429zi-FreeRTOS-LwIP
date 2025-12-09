#ifndef __NETDB__
#define __NETDB__

/* Stub de compatibilidade para netdb em ambiente LwIP + bare-metal.
 * Reexporta a API do LwIP e garante a presença de símbolos mínimos
 * usados pelo open62541 (gai_strerror, NI_NUMERICHOST), caso a
 * implementação do LwIP não os forneça.
 */

#include "lwip/netdb.h"

#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 0x01
#endif

/* Alguns ports do LwIP podem não fornecer gai_strerror. Fornecemos uma
 * implementação mínima que devolve uma string genérica. O open62541 usa
 * apenas para logging, então isso é suficiente neste firmware. */
#ifndef gai_strerror
static inline const char *gai_strerror(int errcode)
{
    (void)errcode;
    return "gai_strerror not supported";
}
#endif

#endif // __NETDB__