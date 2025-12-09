#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <fcntl.h>

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netif.h"

#include "logger.h"

/*
 * Stubs POSIX mínimos para ambiente bare-metal + LwIP.
 * São usados apenas por caminhos de logging/event loop do open62541.
 * Não implementam funcionalidade real de hostname/pipe.
 */

/* clock_gettime: aproximação usando gettimeofday() como base.
 * Ignora o clock_id e devolve um tempo crescente em nanossegundos.
 */
int __attribute__((weak)) clock_gettime(clockid_t clk_id, struct timespec *ts) {
    (void)clk_id;
    if (!ts) {
        errno = EINVAL;
        return -1;
    }

    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        /* Se gettimeofday não estiver disponível, devolve zero. */
        ts->tv_sec = 0;
        ts->tv_nsec = 0;
        return -1;
    }

    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = (long)tv.tv_usec * 1000L;
    return 0;
}

/* getnameinfo: stub que devolve um hostname genérico ou vazio.
 * Usado apenas para mensagens de log no open62541.
 */
int __attribute__((weak)) getnameinfo(const struct sockaddr *sa, socklen_t salen,
                                      char *host, size_t hostlen,
                                      char *serv, size_t servlen, int flags) {
    (void)sa;
    (void)salen;
    (void)flags;

    if (host && hostlen > 0) {
        /* Hostname genérico; garante string terminada em NUL. */
        const char *dummy = "0.0.0.0";
        size_t len = strlen(dummy);
        if (len >= hostlen) {
            len = hostlen - 1;
        }
        memcpy(host, dummy, len);
        host[len] = '\0';
    }

    if (serv && servlen > 0) {
        serv[0] = '\0';
    }

    return 0;
}

/* gethostname: stub simples que devolve um nome fixo para o dispositivo. */
int __attribute__((weak)) gethostname(char *name, size_t len) {
    if (!name || len == 0) {
        errno = EINVAL;
        return -1;
    }

    const char *dummy = "stm32";
    size_t n = strlen(dummy);
    if (n >= len) {
        n = len - 1;
    }
    memcpy(name, dummy, n);
    name[n] = '\0';
    return 0;
}

/* pipe: implementação baseada em sockets UDP da LwIP para satisfazer o EventLoop POSIX do open62541.
 * O open62541 utiliza pipe apenas para um mecanismo interno de interrupção
 * no EventLoop POSIX (self-pipe trick). Aqui usamos um par de sockets UDP
 * onde o descritor de escrita envia datagramas para o descritor de leitura.
 */
int __attribute__((weak)) pipe(int pipefd[2]) {
	if (!pipefd) {
		errno = EINVAL;
		loggerPrint("[posix_stubs] pipe(): argumento invalido (pipefd NULL)\n");
		return -1;
	}

	/* Socket de leitura (destino dos datagramas) */
	int s2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s2 < 0) {
		loggerPrint("[posix_stubs] pipe(): socket(s2 UDP) falhou, errno=%d\n", errno);
		return -1;
	}

	struct sockaddr_in addr;
	socklen_t addrlen = (socklen_t)sizeof(addr);
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = 0; /* porta efemera */

	/* Usar o IP da interface padrao (netif_default) se disponivel; caso
	 * contrario, cair para INADDR_ANY como fallback. */
	const ip4_addr_t *ip = NULL;
	if (netif_default != NULL) {
		ip = netif_ip4_addr(netif_default);
	}
	if (ip != NULL && ip->addr != 0) {
		addr.sin_addr.s_addr = ip->addr;
	} else {
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	if (bind(s2, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		loggerPrint("[posix_stubs] pipe(): bind(s2) falhou, errno=%d\n", errno);
		closesocket(s2);
		return -1;
	}

	if (getsockname(s2, (struct sockaddr *)&addr, &addrlen) < 0) {
		loggerPrint("[posix_stubs] pipe(): getsockname(s2) falhou, errno=%d\n", errno);
		closesocket(s2);
		return -1;
	}

	/* Socket de escrita (origem dos datagramas) */
	int s1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s1 < 0) {
		loggerPrint("[posix_stubs] pipe(): socket(s1 UDP) falhou, errno=%d\n", errno);
		closesocket(s2);
		return -1;
	}

	if (connect(s1, (struct sockaddr *)&addr, addrlen) < 0) {
		loggerPrint("[posix_stubs] pipe(): connect(s1 UDP) falhou, errno=%d\n", errno);
		closesocket(s1);
		closesocket(s2);
		return -1;
	}

	pipefd[0] = s2; /* leitura */
	pipefd[1] = s1; /* escrita */

	loggerPrint("[posix_stubs] pipe(): criado par UDP rfd=%d, wfd=%d, addr=0x%08lx, port=%u\n",
	           s2, s1, (unsigned long)addr.sin_addr.s_addr, (unsigned)ntohs(addr.sin_port));
	return 0;
}

/* fcntl: stub POSIX que delega para lwip_fcntl a fim de configurar
 * descritores de socket do LwIP (por exemplo, O_NONBLOCK). O open62541
 * utiliza apenas F_GETFL e F_SETFL neste firmware.
 */
int __attribute__((weak)) fcntl(int s, int cmd, ...)
{
	int val = 0;

	/* F_GETFL e outros comandos que nao exigem terceiro parametro
	 * nao devem consumir argumentos variadicos. */
	if (cmd != F_GETFL)
	{
		va_list ap;
		va_start(ap, cmd);
		val = va_arg(ap, int);
		va_end(ap);
	}

	int res = lwip_fcntl(s, cmd, val);
	if (res < 0 && errno == ENOSYS && (cmd == F_GETFL || cmd == F_SETFL))
	{
		/* Alguns ports do LwIP nao implementam fcntl, retornando ENOSYS.
		 * Para o open62541, isso so e usado para configurar O_NONBLOCK.
		 * Aqui simulamos sucesso para nao derrubar o EventLoop, deixando
		 * o socket com comportamento padrao do LwIP. */
		errno = 0;
		return (cmd == F_GETFL) ? 0 : 0;
	}

	return res;
}
