/**
 * @file httpserver.c
 * @brief Servidor HTTP integrado para STM32F429ZI.
 *
 * Este arquivo implementa a inicialização e configuração do servidor HTTP
 * usando a biblioteca httpd do LwIP. Serve uma landing page e uma página 404.
 *
 * @note O servidor utiliza o sistema de arquivos embarcado (fsdata) do LwIP.
 */

#include "lwip/opt.h"
#include "lwip/apps/httpd.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/ip4_addr.h"
#include "lwip/netifapi.h"

#include "ethernetif.h"
#include "httpserver.h"
#include "webpages.h"

#include "FreeRTOS.h"
#include "task.h"

#include <string.h>
#include <stdio.h>

/*-----------------------------------------------------------------------------
 * Variáveis Estáticas
 *----------------------------------------------------------------------------*/

/**
 * @brief Interface de rede principal.
 */
static struct netif httpNetif;

/**
 * @brief Flag indicando se a rede foi inicializada.
 */
static volatile uint8_t networkInitialized = 0;

/*-----------------------------------------------------------------------------
 * Protótipos de Funções Privadas
 *----------------------------------------------------------------------------*/

static void networkStatusCallback(struct netif *netif);
static void networkLinkCallback(struct netif *netif);
static void dhcpClientTask(void *pvParameters);

/*-----------------------------------------------------------------------------
 * Implementação das Funções Públicas
 *----------------------------------------------------------------------------*/

/**
 * @brief Callback chamado quando tcpip_init() finaliza.
 */
static volatile uint8_t tcpipInitDone = 0;

static void tcpipInitDoneCallback(void *arg)
{
    (void)arg;
    tcpipInitDone = 1;
    printf("[HTTP] Stack TCP/IP inicializado\n");
}

/**
 * @brief Inicializa o stack de rede e o servidor HTTP.
 *
 * Esta função deve ser chamada após a inicialização do FreeRTOS.
 * Configura a pilha TCP/IP, adiciona a interface de rede e inicia
 * o servidor HTTP.
 *
 * @return 0 se sucesso, -1 se erro.
 */
int httpServerInit(void)
{
    ip4_addr_t ipaddr;
    ip4_addr_t netmask;
    ip4_addr_t gateway;

    printf("[HTTP] Inicializando stack TCP/IP...\n\r");

    /* Inicializar stack TCP/IP com callback */
    tcpip_init(tcpipInitDoneCallback, NULL);

    /* Aguardar inicialização do stack (máximo 5 segundos) */
    uint32_t waitCount = 0;
    while (!tcpipInitDone && waitCount < 50)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        waitCount++;
    }

    if (!tcpipInitDone)
    {
        printf("[HTTP] ERRO: Timeout na inicializacao do TCP/IP\n\r");
        return -1;
    }

    /* Configurar IP inicial como 0.0.0.0 para forçar DHCP */
    IP4_ADDR(&ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&netmask, 0, 0, 0, 0);
    IP4_ADDR(&gateway, 0, 0, 0, 0);

    printf("[HTTP] Adicionando interface de rede...\n\r");

    /* Adicionar interface de rede */
    if (netif_add(&httpNetif, &ipaddr, &netmask, &gateway,
                  NULL, ethernetIfInit, tcpip_input) == NULL)
    {
        printf("[HTTP] ERRO: Falha ao adicionar netif\n\r");
        return -1;
    }

    /* Configurar hostname para identificação no DHCP */
#if LWIP_NETIF_HOSTNAME
    netif_set_hostname(&httpNetif, "stm32-http");
#endif

    /* Registrar callbacks de status */
    netif_set_status_callback(&httpNetif, networkStatusCallback);
    netif_set_link_callback(&httpNetif, networkLinkCallback);

    /* Definir como interface padrão */
    netif_set_default(&httpNetif);

    /* Ativar interface */
    netif_set_up(&httpNetif);

    printf("[HTTP] Interface de rede configurada\n\r");

    /* Criar tarefa para monitorar DHCP */
    xTaskCreate(dhcpClientTask,
                "DHCP",
                512,  /* Aumentado para mais logs */
                NULL,
                2,    /* Prioridade maior */
                NULL);

    /* Inicializar servidor HTTP */
    httpd_init();

    networkInitialized = 1;

    printf("[HTTP] Servidor HTTP inicializado\n\r");

    return 0;
}

/**
 * @brief Retorna o endereço IP atual da interface.
 *
 * @param ipStr Buffer para armazenar o IP em formato string (mín. 16 bytes).
 * @return 1 se IP válido obtido, 0 se não há IP.
 */
int httpServerGetIpAddress(char *ipStr)
{
    if (!networkInitialized || ipStr == NULL)
    {
        return 0;
    }

    const ip4_addr_t *ip = netif_ip4_addr(&httpNetif);
    if (ip->addr == 0)
    {
        return 0;
    }

    snprintf(ipStr, 16, "%d.%d.%d.%d",
             (int)(ip->addr & 0xFF),
             (int)((ip->addr >> 8) & 0xFF),
             (int)((ip->addr >> 16) & 0xFF),
             (int)((ip->addr >> 24) & 0xFF));

    return 1;
}

/**
 * @brief Retorna a estrutura netif da interface HTTP.
 *
 * @return Ponteiro para a estrutura netif.
 */
struct netif *httpServerGetNetif(void)
{
    return &httpNetif;
}

/**
 * @brief Verifica se a rede está inicializada e conectada.
 *
 * @return 1 se conectado, 0 se desconectado.
 */
int httpServerIsConnected(void)
{
    if (!networkInitialized)
    {
        return 0;
    }

    return netif_is_up(&httpNetif) && netif_is_link_up(&httpNetif);
}

/*-----------------------------------------------------------------------------
 * Implementação das Funções Privadas
 *----------------------------------------------------------------------------*/

/**
 * @brief Callback chamado quando o status da interface muda.
 *
 * @param netif Ponteiro para a interface de rede.
 */
static void networkStatusCallback(struct netif *netif)
{
    if (netif_is_up(netif))
    {
        const ip4_addr_t *ip = netif_ip4_addr(netif);
        
        /* Log do IP obtido (pode ser redirecionado para UART) */
        printf("Network up: %d.%d.%d.%d\n\r",
               (int)(ip->addr & 0xFF),
               (int)((ip->addr >> 8) & 0xFF),
               (int)((ip->addr >> 16) & 0xFF),
               (int)((ip->addr >> 24) & 0xFF));
    }
    else
    {
        printf("Network down\n\r");
    }
}

/**
 * @brief Callback chamado quando o status do link muda.
 *
 * @param netif Ponteiro para a interface de rede.
 */
static void networkLinkCallback(struct netif *netif)
{
    if (netif_is_link_up(netif))
    {
        printf("Ethernet link up\n\r");
    }
    else
    {
        printf("Ethernet link down\n\r");
    }
}

/**
 * @brief Tarefa para monitorar o cliente DHCP.
 *
 * Monitora o estado do DHCP, verifica o link e gerencia reconexões.
 *
 * @param pvParameters Parâmetros (não utilizado).
 */
static void dhcpClientTask(void *pvParameters)
{
    (void)pvParameters;

    static uint8_t lastLinkState = 0;
    static uint8_t dhcpStarted = 0;
    static uint32_t dhcpWaitCounter = 0;
    static uint8_t ipObtained = 0;

    /* Aguardar um pouco antes de começar */
    vTaskDelay(pdMS_TO_TICKS(500));

    printf("[DHCP] Tarefa de monitoramento iniciada\n\r");

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        uint8_t currentLinkState = netif_is_link_up(&httpNetif) ? 1 : 0;

        /* Detectar mudança de estado do link */
        if (currentLinkState != lastLinkState)
        {
            lastLinkState = currentLinkState;

            if (currentLinkState)
            {
                printf("[DHCP] Link ativo - iniciando cliente DHCP\n\r");
                netifapi_dhcp_start(&httpNetif);
                dhcpStarted = 1;
                dhcpWaitCounter = 0;
                ipObtained = 0;
            }
            else
            {
                printf("[DHCP] Link perdido - parando DHCP\n\r");
                netifapi_dhcp_stop(&httpNetif);
                dhcpStarted = 0;
                ipObtained = 0;
            }
        }

        /* Se link está ativo, monitorar DHCP */
        if (currentLinkState && dhcpStarted)
        {
            const ip4_addr_t *ip = netif_ip4_addr(&httpNetif);

            if (ip->addr != 0 && !ipObtained)
            {
                /* IP obtido pela primeira vez */
                ipObtained = 1;
                printf("[DHCP] IP obtido: %d.%d.%d.%d\n\r",
                       (int)(ip->addr & 0xFF),
                       (int)((ip->addr >> 8) & 0xFF),
                       (int)((ip->addr >> 16) & 0xFF),
                       (int)((ip->addr >> 24) & 0xFF));

                const ip4_addr_t *gw = netif_ip4_gw(&httpNetif);
                printf("[DHCP] Gateway: %d.%d.%d.%d\n\r",
                       (int)(gw->addr & 0xFF),
                       (int)((gw->addr >> 8) & 0xFF),
                       (int)((gw->addr >> 16) & 0xFF),
                       (int)((gw->addr >> 24) & 0xFF));

                const ip4_addr_t *nm = netif_ip4_netmask(&httpNetif);
                printf("[DHCP] Mascara: %d.%d.%d.%d\n\r",
                       (int)(nm->addr & 0xFF),
                       (int)((nm->addr >> 8) & 0xFF),
                       (int)((nm->addr >> 16) & 0xFF),
                       (int)((nm->addr >> 24) & 0xFF));
            }
            else if (ip->addr == 0)
            {
                /* Ainda aguardando IP */
                dhcpWaitCounter++;

                /* Log periódico a cada 5 segundos */
                if (dhcpWaitCounter % 5 == 0)
                {
                    struct dhcp *dhcpState = netif_dhcp_data(&httpNetif);
                    if (dhcpState != NULL)
                    {
                        printf("[DHCP] Aguardando IP... (tentativa %d, estado %d)\n\r",
                               dhcpState->tries, dhcpState->state);
                    }
                    else
                    {
                        printf("[DHCP] Aguardando IP... (sem estado DHCP)\n\r");
                    }
                }

                /* Timeout: 30 segundos sem IP -> usar fallback */
                if (dhcpWaitCounter > 30)
                {
                    printf("[DHCP] Timeout! Usando IP estatico fallback\n\r");

                    netifapi_dhcp_stop(&httpNetif);

                    ip4_addr_t fallbackIp, fallbackNetmask, fallbackGw;
                    IP4_ADDR(&fallbackIp, 192, 168, 0, 228);
                    IP4_ADDR(&fallbackNetmask, 255, 255, 255, 0);
                    IP4_ADDR(&fallbackGw, 192, 168, 0, 1);

                    netif_set_addr(&httpNetif, &fallbackIp, &fallbackNetmask, &fallbackGw);

                    dhcpStarted = 0;
                    ipObtained = 1;

                    printf("[DHCP] IP fallback: 192.168.0.228\n\r");
                }
            }
        }
    }
}
