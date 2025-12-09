#ifndef SYS_IOCTL_H_
#define SYS_IOCTL_H_

/* Stub de compatibilidade para builds bare-metal (sem suporte real a ioctl).
 * O open62541 pode incluir <sys/ioctl.h> em caminhos de código POSIX que não
 * são usados nesta arquitetura. Manter vazio evita erro de header ausente.
 */

#endif /* SYS_IOCTL_H_ */
