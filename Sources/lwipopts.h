/**
 * @file lwipopts.h
 * @brief Arquivo de configuração do LwIP para STM32F429ZI com FreeRTOS.
 *
 * Este arquivo define os parâmetros de configuração da pilha TCP/IP LwIP,
 * otimizada para uso com FreeRTOS em microcontroladores STM32F4.
 *
 * @note Consulte a documentação oficial do LwIP para detalhes de cada opção:
 *       https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html
 */

#ifndef LWIPOPTS_H
#define LWIPOPTS_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------
 * Configurações de Plataforma
 *----------------------------------------------------------------------------*/

/**
 * @brief Indica que não há sistema operacional (valor 0) ou há SO (valor 1).
 *        Como usamos FreeRTOS, definimos como 1.
 */
#define NO_SYS                          0

/**
 * @brief Habilita verificação de limites em operações críticas.
 */
#define LWIP_NOASSERT                   0

/**
 * @brief Define o nível de depuração (0 = desabilitado).
 */
#define LWIP_DEBUG                      0

/**
 * @brief Faz o LwIP fornecer definições de errno (ENOMEM, ENOBUFS, etc.).
 *        Necessário em sistemas embarcados sem biblioteca C completa.
 */
/* Usar errno da libc (newlib) */
#define LWIP_PROVIDE_ERRNO              0
#define LWIP_ERRNO_STDINCLUDE           1

/* Usar struct timeval/FD_SET da libc */
#define LWIP_TIMEVAL_PRIVATE            0

/*-----------------------------------------------------------------------------
 * Configurações de Memória
 *----------------------------------------------------------------------------*/

/**
 * @brief Tamanho do heap de memória do LwIP em bytes.
 *        16KB é suficiente para aplicações HTTP básicas.
 */
#define MEM_SIZE                        (16 * 1024)

/**
 * @brief Alinhamento de memória em bytes.
 */
#define MEM_ALIGNMENT                   4

/**
 * @brief Número de buffers de memória para recepção de pacotes.
 */
#define MEMP_NUM_PBUF                   16

/**
 * @brief Número máximo de conexões UDP simultâneas.
 */
#define MEMP_NUM_UDP_PCB                4

/**
 * @brief Número máximo de conexões TCP simultâneas.
 */
#define MEMP_NUM_TCP_PCB                8

/**
 * @brief Número máximo de conexões TCP em estado LISTEN.
 */
#define MEMP_NUM_TCP_PCB_LISTEN         4

/**
 * @brief Número de segmentos TCP enfileirados para envio.
 */
#define MEMP_NUM_TCP_SEG                16

/**
 * @brief Número de estruturas netbuf.
 */
#define MEMP_NUM_NETBUF                 8

/**
 * @brief Número de estruturas netconn.
 */
#define MEMP_NUM_NETCONN                8

/**
 * @brief Número de entradas na tabela ARP.
 */
#define MEMP_NUM_ARP_QUEUE              8

/**
 * @brief Número de mensagens TCPIP.
 */
#define MEMP_NUM_TCPIP_MSG_INPKT        16

/*-----------------------------------------------------------------------------
 * Configurações de pbuf (buffers de pacotes)
 *----------------------------------------------------------------------------*/

/**
 * @brief Tamanho do pool de pbufs.
 */
#define PBUF_POOL_SIZE                  16

/**
 * @brief Tamanho de cada pbuf no pool.
 *        1524 bytes = MTU (1500) + cabeçalhos Ethernet (14) + padding.
 */
#define PBUF_POOL_BUFSIZE               1524

/*-----------------------------------------------------------------------------
 * Configurações ARP
 *----------------------------------------------------------------------------*/

/**
 * @brief Habilita o protocolo ARP.
 */
#define LWIP_ARP                        1

/**
 * @brief Tamanho da tabela ARP.
 */
#define ARP_TABLE_SIZE                  10

/**
 * @brief Tempo em segundos para expiração de entrada ARP.
 */
#define ARP_MAXAGE                      300

/*-----------------------------------------------------------------------------
 * Configurações IP
 *----------------------------------------------------------------------------*/

/**
 * @brief Habilita encaminhamento de pacotes IP (desabilitado para host simples).
 */
#define IP_FORWARD                      0

/**
 * @brief Habilita reassembly de fragmentos IP.
 */
#define IP_REASSEMBLY                   1

/**
 * @brief Habilita fragmentação IP.
 */
#define IP_FRAG                         1

/**
 * @brief Tamanho máximo para reassembly.
 */
#define IP_REASS_MAX_PBUFS              10

/**
 * @brief TTL padrão para pacotes IP.
 */
#define IP_DEFAULT_TTL                  255

/*-----------------------------------------------------------------------------
 * Configurações ICMP
 *----------------------------------------------------------------------------*/

/**
 * @brief Habilita ICMP (necessário para ping).
 */
#define LWIP_ICMP                       1

/*-----------------------------------------------------------------------------
 * Configurações DHCP
 *----------------------------------------------------------------------------*/

/**
 * @brief Habilita cliente DHCP para obter IP automaticamente.
 */
#define LWIP_DHCP                       1

/**
 * @brief Verificação ARP após obter IP (evita conflitos).
 */
#define DHCP_DOES_ARP_CHECK             1

/**
 * @brief Cria hostname para DHCP (identificação na rede).
 */
#define LWIP_NETIF_HOSTNAME             1

/*-----------------------------------------------------------------------------
 * Configurações UDP
 *----------------------------------------------------------------------------*/

/**
 * @brief Habilita protocolo UDP.
 */
#define LWIP_UDP                        1

/*-----------------------------------------------------------------------------
 * Configurações TCP
 *----------------------------------------------------------------------------*/

/**
 * @brief Habilita protocolo TCP.
 */
#define LWIP_TCP                        1

/**
 * @brief Tamanho máximo do segmento TCP (MSS).
 *        1460 = MTU (1500) - cabeçalho IP (20) - cabeçalho TCP (20).
 */
#define TCP_MSS                         1460

/**
 * @brief Tamanho da janela de envio TCP.
 */
#define TCP_SND_BUF                     (4 * TCP_MSS)

/**
 * @brief Tamanho da fila de envio TCP.
 */
#define TCP_SND_QUEUELEN                (4 * TCP_SND_BUF / TCP_MSS)

/**
 * @brief Tamanho da janela de recepção TCP.
 */
#define TCP_WND                         (4 * TCP_MSS)

/**
 * @brief Tempo máximo de vida do segmento (2 minutos).
 */
#define TCP_MAXRTX                      12

/**
 * @brief Máximo de retransmissões SYN.
 */
#define TCP_SYNMAXRTX                   6

/*-----------------------------------------------------------------------------
 * Configurações de Thread/OS (FreeRTOS)
 *----------------------------------------------------------------------------*/

/**
 * @brief Tamanho da pilha da thread TCPIP.
 */
#define TCPIP_THREAD_STACKSIZE          1024

/**
 * @brief Prioridade da thread TCPIP (alta prioridade).
 */
#define TCPIP_THREAD_PRIO               (configMAX_PRIORITIES - 2)

/**
 * @brief Tamanho da caixa de mensagens TCPIP.
 */
#define TCPIP_MBOX_SIZE                 16

/**
 * @brief Tamanho padrão da caixa de mensagens.
 */
#define DEFAULT_ACCEPTMBOX_SIZE         8
#define DEFAULT_RAW_RECVMBOX_SIZE       8
#define DEFAULT_UDP_RECVMBOX_SIZE       8
#define DEFAULT_TCP_RECVMBOX_SIZE       8

/*-----------------------------------------------------------------------------
 * Configurações de API
 *----------------------------------------------------------------------------*/

/**
 * @brief Habilita API netconn (necessária para sockets).
 */
#define LWIP_NETCONN                    1

/**
 * @brief Habilita API de sockets BSD.
 */
#define LWIP_SOCKET                     1

/**
 * @brief Evita redefinir read/write/close/fcntl/poll como macros.
 */
#define LWIP_POSIX_SOCKETS_IO_NAMES     0

/**
 * @brief Habilita API netif.
 */
#define LWIP_NETIF_API                  1

/**
 * @brief Habilita callback de status do link.
 */
#define LWIP_NETIF_LINK_CALLBACK        1

/**
 * @brief Habilita callback de status do netif.
 */
#define LWIP_NETIF_STATUS_CALLBACK      1

/*-----------------------------------------------------------------------------
 * Configurações de Checksum
 *----------------------------------------------------------------------------*/

/**
 * @brief Configuração de checksum por SOFTWARE.
 *        IMPORTANTE: Deve estar alinhado com ETH_CHECKSUM_BY_SOFTWARE no driver.
 */
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1

/**
 * @brief Desabilita offload de checksum pelo hardware Ethernet.
 */
#define CHECKSUM_BY_HARDWARE            0

/*-----------------------------------------------------------------------------
 * Configurações do Servidor HTTP (httpd)
 *----------------------------------------------------------------------------*/

/**
 * @brief Habilita o servidor HTTP integrado do LwIP.
 */
#define LWIP_HTTPD                      1

/**
 * @brief Não usa arquivo fsdata customizado (fsdata_custom.c).
 */
#define HTTPD_USE_CUSTOM_FSDATA         0

/**
 * @brief Habilita funções de filesystem customizado (fs_open_custom, etc.).
 */
#define LWIP_HTTPD_CUSTOM_FILES         1

/**
 * @brief Número máximo de conexões HTTP simultâneas.
 */
#define MEMP_NUM_PARALLEL_HTTPD_CONNS   4

/**
 * @brief Habilita suporte a SSI (Server Side Includes).
 */
#define LWIP_HTTPD_SSI                  0

/**
 * @brief Habilita suporte a CGI (Common Gateway Interface).
 */
#define LWIP_HTTPD_CGI                  0

/**
 * @brief Habilita página 404 customizada.
 */
#define LWIP_HTTPD_SUPPORT_11_KEEPALIVE 0

/**
 * @brief Tamanho máximo do URI.
 */
#define LWIP_HTTPD_MAX_REQUEST_URI_LEN  128

/*-----------------------------------------------------------------------------
 * Configurações do Cliente MQTT
 *----------------------------------------------------------------------------*/

/**
 * @brief Habilita o cliente MQTT do LwIP.
 */
#define LWIP_ALTCP                      0

/**
 * @brief Tamanho do buffer de saída MQTT.
 */
#define MQTT_OUTPUT_RINGBUF_SIZE        256

/**
 * @brief Tamanho do buffer de cabeçalho variável MQTT.
 */
#define MQTT_VAR_HEADER_BUFFER_LEN      128

/**
 * @brief Tamanho máximo do request em voo.
 */
#define MQTT_REQ_MAX_IN_FLIGHT          4

/**
 * @brief Timeout para ciclos de requisição MQTT (segundos).
 */
#define MQTT_REQ_TIMEOUT                30

/**
 * @brief Timeout de conexão MQTT (segundos).
 */
#define MQTT_CONNECT_TIMOUT             100

/*-----------------------------------------------------------------------------
 * Configurações SNTP (NTP)
 *----------------------------------------------------------------------------*/
/* Permite resolver servidores NTP via DNS */
#define SNTP_SERVER_DNS                 1
/* Quantidade de servidores NTP configuráveis */
#define SNTP_MAX_SERVERS                2
/* Intervalo de atualização (ms): 1 hora */
#define SNTP_UPDATE_DELAY               (60 * 60 * 1000)

/* Hook para aplicar horário vindo do SNTP */
#include <sys/time.h>
#define SNTP_SET_SYSTEM_TIME(sec)                       \
    do {                                                \
        struct timeval tv;                              \
        tv.tv_sec = (time_t)(sec);                      \
        tv.tv_usec = 0;                                 \
        settimeofday(&tv, NULL);                        \
    } while (0)

/*-----------------------------------------------------------------------------
 * Configurações de Estatísticas
 *----------------------------------------------------------------------------*/

/**
 * @brief Habilita coleta de estatísticas (desabilitado para economizar RAM).
 */
#define LWIP_STATS                      0
#define LWIP_STATS_DISPLAY              0

/*-----------------------------------------------------------------------------
 * Configurações de DNS
 *----------------------------------------------------------------------------*/

/**
 * @brief Habilita cliente DNS.
 */
#define LWIP_DNS                        1

/**
 * @brief Tamanho máximo do cache DNS.
 */
#define DNS_TABLE_SIZE                  4

/*-----------------------------------------------------------------------------
 * Configurações Ethernet
 *----------------------------------------------------------------------------*/

/**
 * @brief Habilita suporte a Ethernet.
 */
#define LWIP_ETHERNET                   1

/**
 * @brief Entradas ARP estáticas (para gateway fixo).
 */
#define ETHARP_SUPPORT_STATIC_ENTRIES   1

/**
 * @brief Timeout para entradas ARP em segundos.
 */
#define ARP_QUEUEING                    1

/**
 * @brief Função de random para DHCP (xid).
 *        Definida em cc.h - LWIP_RAND() e lwip_rand()
 */

#ifdef __cplusplus
}
#endif

#endif /* LWIPOPTS_H */
