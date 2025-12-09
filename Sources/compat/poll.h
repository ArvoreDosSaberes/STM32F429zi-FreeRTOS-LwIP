#ifndef POLL_H_
#define POLL_H_

/* Stub de compatibilidade para ambientes bare-metal.
 * Quando o LwIP está habilitado (LWIP_SOCKET), ele já define
 * nfds_t, struct pollfd e a função/macro poll() em lwip/sockets.h.
 * Aqui apenas reexportamos esses tipos para satisfazer o include
 * <poll.h> usado pelo open62541.
 */

#include <lwip/sockets.h>

#endif /* POLL_H_ */
