/**
 * @file ethernetif.c
 * @brief Driver de interface Ethernet para LwIP usando STM32 HAL.
 *
 * Este arquivo implementa a interface de rede (netif) entre o LwIP e o
 * controlador Ethernet do STM32F429 usando o HAL oficial da ST.
 */

#include "stm32f4xx_hal.h"

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"

#include "ethernetif.h"
#include "stm32f4xx_hal_conf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <string.h>
#include <stdio.h>

/*-----------------------------------------------------------------------------
 * Definições e Constantes
 *----------------------------------------------------------------------------*/

#define IFNAME0 's'
#define IFNAME1 't'

#define ETHERNETIF_TASK_STACK_SIZE      512
#define ETHERNETIF_TASK_PRIORITY        (configMAX_PRIORITIES - 1)
#define PHY_TIMEOUT_MS                  5000

/* Registradores do PHY são definidos em stm32f4xx_hal_conf.h */
#define PHY_SPECIAL_STATUS              PHY_SR
#define PHY_BCR_SOFT_RESET              PHY_RESET
#define PHY_BCR_AUTONEG_EN              PHY_AUTONEGOTIATION
#define PHY_BCR_AUTONEG_RESTART         PHY_RESTART_AUTONEGOTIATION
#define PHY_BSR_LINK_STATUS             PHY_LINKED_STATUS
#define PHY_BSR_AUTONEG_COMPLETE        PHY_AUTONEGO_COMPLETE

#define PHY_SPEED_MASK                  0x001CU
#define PHY_SPEED_10HD                  0x0004U
#define PHY_SPEED_10FD                  0x0014U
#define PHY_SPEED_100HD                 0x0008U
#define PHY_SPEED_100FD                 0x0018U

/*-----------------------------------------------------------------------------
 * Variáveis Globais
 *----------------------------------------------------------------------------*/

ETH_HandleTypeDef heth;

__attribute__((aligned(4))) ETH_DMADescTypeDef DMARxDscrTab[ETH_RXBUFNB];
__attribute__((aligned(4))) ETH_DMADescTypeDef DMATxDscrTab[ETH_TXBUFNB];

__attribute__((aligned(4))) uint8_t Rx_Buff[ETH_RXBUFNB][ETH_RX_BUF_SIZE];
__attribute__((aligned(4))) uint8_t Tx_Buff[ETH_TXBUFNB][ETH_TX_BUF_SIZE];

static struct netif *ethNetif = NULL;
static SemaphoreHandle_t rxSemaphore = NULL;
static TaskHandle_t rxTaskHandle = NULL;
static volatile uint8_t ethLinkUp = 0;

/*-----------------------------------------------------------------------------
 * Protótipos
 *----------------------------------------------------------------------------*/

static void ethernetIfMspInit(void);
static void ethernetIfInitMac(void);
static int ethernetIfInitPhy(void);
static err_t ethernetIfOutput(struct netif *netif, struct pbuf *p);
static struct pbuf *ethernetIfInput(void);
static void ethernetIfTask(void *pvParameters);

/*-----------------------------------------------------------------------------
 * Implementação
 *----------------------------------------------------------------------------*/

err_t ethernetIfInit(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    ethNetif = netif;

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    netif->output = etharp_output;
    netif->linkoutput = ethernetIfOutput;

    netif->hwaddr_len = ETH_HWADDR_LEN;
    netif->hwaddr[0] = ETH_MAC_ADDR0;
    netif->hwaddr[1] = ETH_MAC_ADDR1;
    netif->hwaddr[2] = ETH_MAC_ADDR2;
    netif->hwaddr[3] = ETH_MAC_ADDR3;
    netif->hwaddr[4] = ETH_MAC_ADDR4;
    netif->hwaddr[5] = ETH_MAC_ADDR5;

    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP;

    ethernetIfMspInit();
    ethernetIfInitMac();

    if (ethernetIfInitPhy() != 0)
    {
        printf("[EthIf] AVISO: PHY nao respondeu corretamente\n\r");
    }

    rxSemaphore = xSemaphoreCreateBinary();
    if (rxSemaphore == NULL)
    {
        printf("[EthIf] ERRO: Falha ao criar semaforo\n\r");
        return ERR_MEM;
    }

    BaseType_t result = xTaskCreate(ethernetIfTask,
                                    "EthRx",
                                    ETHERNETIF_TASK_STACK_SIZE,
                                    NULL,
                                    ETHERNETIF_TASK_PRIORITY,
                                    &rxTaskHandle);

    if (result != pdPASS)
    {
        printf("[EthIf] ERRO: Falha ao criar tarefa\n\r");
        return ERR_MEM;
    }

    /* Habilitar interrupções de recepção */
    __HAL_ETH_DMA_ENABLE_IT(&heth, ETH_DMA_IT_NIS | ETH_DMA_IT_R);

    HAL_ETH_Start(&heth);

    printf("[EthIf] Aguardando link...\n\r");

    /* Aguardar até 3 segundos pelo link */
    uint32_t startTick = HAL_GetTick();
    uint32_t phyReg;
    int linkDetected = 0;

    while ((HAL_GetTick() - startTick) < 3000)
    {
        if (HAL_ETH_ReadPHYRegister(&heth, PHY_BSR, &phyReg) == HAL_OK)
        {
            if (phyReg & PHY_BSR_LINK_STATUS)
            {
                linkDetected = 1;
                break;
            }
        }
        HAL_Delay(100);
    }

    if (linkDetected)
    {
        ethLinkUp = 1;
        netif->flags |= NETIF_FLAG_LINK_UP;
        netif_set_link_up(netif);
        printf("[EthIf] Link detectado!\n\r");
    }
    else
    {
        ethLinkUp = 0;
        netif->flags &= ~NETIF_FLAG_LINK_UP;
        netif_set_link_down(netif);
        printf("[EthIf] AVISO: Link nao detectado (verifique o cabo)\n\r");
    }

    return ERR_OK;
}

uint8_t ethernetIfGetLinkState(void)
{
    return ethLinkUp;
}

static void ethernetIfMspInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_ETH_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_13;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(ETH_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);

    printf("[EthIf] MSP inicializado\n\r");
}

static void ethernetIfInitMac(void)
{
    heth.Instance = ETH;
    heth.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
    heth.Init.Speed = ETH_SPEED_100M;
    heth.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
    heth.Init.PhyAddress = LAN8742A_PHY_ADDRESS;
    heth.Init.MACAddr = ethNetif->hwaddr;
    heth.Init.RxMode = ETH_RXINTERRUPT_MODE;
    heth.Init.ChecksumMode = ETH_CHECKSUM_BY_SOFTWARE;
    heth.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;

    if (HAL_ETH_Init(&heth) != HAL_OK)
    {
        printf("[EthIf] ERRO: HAL_ETH_Init falhou\n\r");
        return;
    }

    HAL_ETH_DMATxDescListInit(&heth, DMATxDscrTab, &Tx_Buff[0][0], ETH_TXBUFNB);
    HAL_ETH_DMARxDescListInit(&heth, DMARxDscrTab, &Rx_Buff[0][0], ETH_RXBUFNB);

    printf("[EthIf] MAC inicializado: %02X:%02X:%02X:%02X:%02X:%02X\n",
           ethNetif->hwaddr[0], ethNetif->hwaddr[1], ethNetif->hwaddr[2],
           ethNetif->hwaddr[3], ethNetif->hwaddr[4], ethNetif->hwaddr[5]);
}

static int ethernetIfInitPhy(void)
{
    uint32_t phyReg;
    uint32_t timeout;

    printf("[EthIf] Inicializando PHY...\n\r");

    if (HAL_ETH_WritePHYRegister(&heth, PHY_BCR, PHY_BCR_SOFT_RESET) != HAL_OK)
    {
        printf("[EthIf] ERRO: Falha ao resetar PHY\n\r");
        return -1;
    }

    timeout = HAL_GetTick() + PHY_TIMEOUT_MS;
    do
    {
        HAL_Delay(10);
        if (HAL_ETH_ReadPHYRegister(&heth, PHY_BCR, &phyReg) != HAL_OK)
        {
            return -1;
        }
    } while ((phyReg & PHY_BCR_SOFT_RESET) && (HAL_GetTick() < timeout));

    if (phyReg & PHY_BCR_SOFT_RESET)
    {
        printf("[EthIf] ERRO: Timeout no reset do PHY\n\r");
        return -1;
    }

    printf("[EthIf] PHY resetado\n\r");

    if (HAL_ETH_WritePHYRegister(&heth, PHY_BCR,
                                  PHY_BCR_AUTONEG_EN | PHY_BCR_AUTONEG_RESTART) != HAL_OK)
    {
        printf("[EthIf] ERRO: Falha ao iniciar auto-negociacao\n\r");
        return -1;
    }

    printf("[EthIf] Auto-negociacao iniciada\n\r");

    timeout = HAL_GetTick() + PHY_TIMEOUT_MS;
    do
    {
        HAL_Delay(100);
        if (HAL_ETH_ReadPHYRegister(&heth, PHY_BSR, &phyReg) != HAL_OK)
        {
            return -1;
        }

        if ((phyReg & PHY_BSR_LINK_STATUS) && (phyReg & PHY_BSR_AUTONEG_COMPLETE))
        {
            break;
        }
    } while (HAL_GetTick() < timeout);

    if (HAL_ETH_ReadPHYRegister(&heth, PHY_BSR, &phyReg) == HAL_OK)
    {
        if (phyReg & PHY_BSR_LINK_STATUS)
        {
            if (HAL_ETH_ReadPHYRegister(&heth, PHY_SPECIAL_STATUS, &phyReg) == HAL_OK)
            {
                switch (phyReg & PHY_SPEED_MASK)
                {
                    case PHY_SPEED_100FD:
                        printf("[EthIf] Velocidade: 100 Mbps Full Duplex\n\r");
                        break;
                    case PHY_SPEED_100HD:
                        printf("[EthIf] Velocidade: 100 Mbps Half Duplex\n\r");
                        break;
                    case PHY_SPEED_10FD:
                        printf("[EthIf] Velocidade: 10 Mbps Full Duplex\n\r");
                        break;
                    case PHY_SPEED_10HD:
                        printf("[EthIf] Velocidade: 10 Mbps Half Duplex\n\r");
                        break;
                    default:
                        printf("[EthIf] Velocidade desconhecida\n\r");
                        break;
                }
            }
        }
        else
        {
            printf("[EthIf] Link nao detectado\n\r");
        }
    }

    return 0;
}

static err_t ethernetIfOutput(struct netif *netif, struct pbuf *p)
{
    err_t errval;
    struct pbuf *q;
    uint8_t *buffer;
    __IO ETH_DMADescTypeDef *DmaTxDesc;
    uint32_t framelength = 0;
    uint32_t bufferoffset = 0;
    uint32_t byteslefttocopy = 0;
    uint32_t payloadoffset = 0;

    (void)netif;

    DmaTxDesc = heth.TxDesc;
    buffer = (uint8_t *)(DmaTxDesc->Buffer1Addr);

    for (q = p; q != NULL; q = q->next)
    {
        if ((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != 0)
        {
            errval = ERR_USE;
            goto error;
        }

        byteslefttocopy = q->len;
        payloadoffset = 0;

        while ((byteslefttocopy + bufferoffset) > ETH_TX_BUF_SIZE)
        {
            memcpy((uint8_t *)((uint8_t *)buffer + bufferoffset),
                   (uint8_t *)((uint8_t *)q->payload + payloadoffset),
                   (ETH_TX_BUF_SIZE - bufferoffset));

            DmaTxDesc = (ETH_DMADescTypeDef *)(DmaTxDesc->Buffer2NextDescAddr);

            if ((DmaTxDesc->Status & ETH_DMATXDESC_OWN) != 0)
            {
                errval = ERR_USE;
                goto error;
            }

            buffer = (uint8_t *)(DmaTxDesc->Buffer1Addr);

            byteslefttocopy = byteslefttocopy - (ETH_TX_BUF_SIZE - bufferoffset);
            payloadoffset = payloadoffset + (ETH_TX_BUF_SIZE - bufferoffset);
            framelength = framelength + (ETH_TX_BUF_SIZE - bufferoffset);
            bufferoffset = 0;
        }

        memcpy((uint8_t *)((uint8_t *)buffer + bufferoffset),
               (uint8_t *)((uint8_t *)q->payload + payloadoffset),
               byteslefttocopy);
        bufferoffset = bufferoffset + byteslefttocopy;
        framelength = framelength + byteslefttocopy;
    }

    HAL_ETH_TransmitFrame(&heth, framelength);
    errval = ERR_OK;

error:
    if ((heth.Instance->DMASR & ETH_DMASR_TUS) != 0)
    {
        heth.Instance->DMASR = ETH_DMASR_TUS;
        heth.Instance->DMATPDR = 0;
    }

    return errval;
}

static struct pbuf *ethernetIfInput(void)
{
    struct pbuf *p = NULL;
    struct pbuf *q;
    uint16_t len;
    uint8_t *buffer;
    __IO ETH_DMADescTypeDef *dmarxdesc;
    uint32_t bufferoffset = 0;
    uint32_t payloadoffset = 0;
    uint32_t byteslefttocopy = 0;

    if (HAL_ETH_GetReceivedFrame(&heth) != HAL_OK)
    {
        return NULL;
    }

    len = heth.RxFrameInfos.length;
    buffer = (uint8_t *)heth.RxFrameInfos.buffer;

    if (len > 0)
    {
        p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    }

    if (p != NULL)
    {
        dmarxdesc = heth.RxFrameInfos.FSRxDesc;
        bufferoffset = 0;

        for (q = p; q != NULL; q = q->next)
        {
            byteslefttocopy = q->len;
            payloadoffset = 0;

            while ((byteslefttocopy + bufferoffset) > ETH_RX_BUF_SIZE)
            {
                memcpy((uint8_t *)((uint8_t *)q->payload + payloadoffset),
                       (uint8_t *)((uint8_t *)buffer + bufferoffset),
                       (ETH_RX_BUF_SIZE - bufferoffset));

                dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
                buffer = (uint8_t *)(dmarxdesc->Buffer1Addr);

                byteslefttocopy = byteslefttocopy - (ETH_RX_BUF_SIZE - bufferoffset);
                payloadoffset = payloadoffset + (ETH_RX_BUF_SIZE - bufferoffset);
                bufferoffset = 0;
            }

            memcpy((uint8_t *)((uint8_t *)q->payload + payloadoffset),
                   (uint8_t *)((uint8_t *)buffer + bufferoffset),
                   byteslefttocopy);
            bufferoffset = bufferoffset + byteslefttocopy;
        }
    }

    dmarxdesc = heth.RxFrameInfos.FSRxDesc;
    for (uint32_t i = 0; i < heth.RxFrameInfos.SegCount; i++)
    {
        dmarxdesc->Status |= ETH_DMARXDESC_OWN;
        dmarxdesc = (ETH_DMADescTypeDef *)(dmarxdesc->Buffer2NextDescAddr);
    }

    heth.RxFrameInfos.SegCount = 0;

    if ((heth.Instance->DMASR & ETH_DMASR_RBUS) != 0)
    {
        heth.Instance->DMASR = ETH_DMASR_RBUS;
        heth.Instance->DMARPDR = 0;
    }

    return p;
}

static void ethernetIfTask(void *pvParameters)
{
    struct pbuf *p;
    uint32_t linkCheckCounter = 0;

    (void)pvParameters;

    for (;;)
    {
        if (xSemaphoreTake(rxSemaphore, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            do
            {
                p = ethernetIfInput();
                if (p != NULL)
                {
                    if (ethNetif->input(p, ethNetif) != ERR_OK)
                    {
                        pbuf_free(p);
                    }
                }
            } while (p != NULL);
        }

        linkCheckCounter++;
        if (linkCheckCounter >= 10)
        {
            linkCheckCounter = 0;

            uint32_t phyReg;
            if (HAL_ETH_ReadPHYRegister(&heth, PHY_BSR, &phyReg) == HAL_OK)
            {
                if ((phyReg & PHY_BSR_LINK_STATUS) && !ethLinkUp)
                {
                    ethLinkUp = 1;
                    netif_set_link_up(ethNetif);
                    printf("[EthIf] Link estabelecido\n\r");
                }
                else if (!(phyReg & PHY_BSR_LINK_STATUS) && ethLinkUp)
                {
                    ethLinkUp = 0;
                    netif_set_link_down(ethNetif);
                    printf("[EthIf] Link perdido\n\r");
                }
            }
        }
    }
}

void ETH_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&heth);
}

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *hethptr)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    (void)hethptr;

    if (rxSemaphore != NULL)
    {
        xSemaphoreGiveFromISR(rxSemaphore, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
