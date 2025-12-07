/**
 * @file stm32f4xx_hal_eth.c
 * @brief Inicialização de hardware Ethernet para STM32F429ZI.
 *
 * Este arquivo contém as funções de configuração de clocks e GPIOs
 * necessárias para o funcionamento do controlador Ethernet do STM32F429.
 *
 * @note O STM32F429ZI-NUCLEO utiliza interface RMII com o PHY LAN8742A.
 */

#include <stdint.h>
#include <stdio.h>

/*-----------------------------------------------------------------------------
 * Definições do PHY LAN8742A
 *----------------------------------------------------------------------------*/

/**
 * @brief Endereço do PHY no barramento MDIO.
 *        O LAN8742A usa endereço 0 por padrão no NUCLEO-F429ZI.
 */
#define PHY_ADDRESS             0

/**
 * @brief Registradores do PHY (padrão IEEE 802.3).
 */
#define PHY_BCR                 0x00    /**< Basic Control Register */
#define PHY_BSR                 0x01    /**< Basic Status Register */
#define PHY_IDR1                0x02    /**< PHY Identifier 1 */
#define PHY_IDR2                0x03    /**< PHY Identifier 2 */
#define PHY_SPECIAL_CTRL_STATUS 0x1F    /**< Special Control/Status (LAN8742A) */

/**
 * @brief Bits do BCR (Basic Control Register).
 */
#define PHY_BCR_SOFT_RESET      (1U << 15)  /**< Soft reset */
#define PHY_BCR_LOOPBACK        (1U << 14)  /**< Loopback mode */
#define PHY_BCR_SPEED_100       (1U << 13)  /**< Speed select: 1=100Mbps */
#define PHY_BCR_AUTONEG_EN      (1U << 12)  /**< Auto-negotiation enable */
#define PHY_BCR_POWER_DOWN      (1U << 11)  /**< Power down */
#define PHY_BCR_ISOLATE         (1U << 10)  /**< Electrical isolation */
#define PHY_BCR_AUTONEG_RESTART (1U << 9)   /**< Restart auto-negotiation */
#define PHY_BCR_DUPLEX_FULL     (1U << 8)   /**< Duplex mode: 1=full */

/**
 * @brief Bits do BSR (Basic Status Register).
 */
#define PHY_BSR_100BASETX_FD    (1U << 14)  /**< 100BASE-TX full duplex capable */
#define PHY_BSR_100BASETX_HD    (1U << 13)  /**< 100BASE-TX half duplex capable */
#define PHY_BSR_10BASET_FD      (1U << 12)  /**< 10BASE-T full duplex capable */
#define PHY_BSR_10BASET_HD      (1U << 11)  /**< 10BASE-T half duplex capable */
#define PHY_BSR_AUTONEG_COMPLETE (1U << 5)  /**< Auto-negotiation complete */
#define PHY_BSR_LINK_STATUS     (1U << 2)   /**< Link status */

/**
 * @brief Bits do Special Control/Status Register (LAN8742A específico).
 */
#define PHY_SCSR_SPEED_MASK     (0x1C)      /**< Speed indication bits */
#define PHY_SCSR_10HD           (0x04)      /**< 10Mbps Half Duplex */
#define PHY_SCSR_10FD           (0x14)      /**< 10Mbps Full Duplex */
#define PHY_SCSR_100HD          (0x08)      /**< 100Mbps Half Duplex */
#define PHY_SCSR_100FD          (0x18)      /**< 100Mbps Full Duplex */

/**
 * @brief ID esperado do LAN8742A.
 */
#define LAN8742A_PHY_ID1        0x0007
#define LAN8742A_PHY_ID2        0xC130

/**
 * @brief Timeout para operações do PHY (em iterações).
 */
#define PHY_TIMEOUT             0x000FFFFF

/*-----------------------------------------------------------------------------
 * Definições de Registradores
 *----------------------------------------------------------------------------*/

/**
 * @brief Endereços base dos periféricos.
 */
#define RCC_BASE            0x40023800UL
#define GPIOA_BASE          0x40020000UL
#define GPIOB_BASE          0x40020400UL
#define GPIOC_BASE          0x40020800UL
#define GPIOG_BASE          0x40021800UL
#define SYSCFG_BASE         0x40013800UL

/**
 * @brief Registradores RCC (Reset and Clock Control).
 */
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_AHB1RSTR        (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x44))

/**
 * @brief Endereço base do Ethernet MAC para acesso MDIO.
 */
#define ETH_BASE            0x40028000UL
#define ETH_MACMIIAR        (*(volatile uint32_t *)(ETH_BASE + 0x10))
#define ETH_MACMIIDR        (*(volatile uint32_t *)(ETH_BASE + 0x14))
#define ETH_MACCR           (*(volatile uint32_t *)(ETH_BASE + 0x00))

/**
 * @brief Bits do registrador MACMIIAR.
 */
#define ETH_MACMIIAR_MB     (1U << 0)   /**< MII Busy */
#define ETH_MACMIIAR_MW     (1U << 1)   /**< MII Write */
#define ETH_MACMIIAR_CR_Pos 2           /**< Clock range position */
#define ETH_MACMIIAR_MR_Pos 6           /**< MII Register address position */
#define ETH_MACMIIAR_PA_Pos 11          /**< PHY Address position */

/**
 * @brief Clock range para MDIO (para HCLK de 82 MHz -> CR = 100b = 4).
 *        HCLK/42 válido para 60-100 MHz.
 */
#define ETH_MACMIIAR_CR_HCLK_DIV42  (0x04U << ETH_MACMIIAR_CR_Pos)

/**
 * @brief Bits de habilitação de clock AHB1.
 */
#define RCC_AHB1ENR_GPIOAEN     (1U << 0)
#define RCC_AHB1ENR_GPIOBEN     (1U << 1)
#define RCC_AHB1ENR_GPIOCEN     (1U << 2)
#define RCC_AHB1ENR_GPIOGEN     (1U << 6)
#define RCC_AHB1ENR_ETHMACEN    (1U << 25)
#define RCC_AHB1ENR_ETHMACTXEN  (1U << 26)
#define RCC_AHB1ENR_ETHMACRXEN  (1U << 27)

/**
 * @brief Bits de reset AHB1.
 */
#define RCC_AHB1RSTR_ETHMACRST  (1U << 25)

/**
 * @brief Bits de habilitação de clock APB2.
 */
#define RCC_APB2ENR_SYSCFGEN    (1U << 14)

/**
 * @brief Estrutura de registradores GPIO.
 */
typedef struct
{
    volatile uint32_t MODER;    /**< Mode register */
    volatile uint32_t OTYPER;   /**< Output type register */
    volatile uint32_t OSPEEDR;  /**< Output speed register */
    volatile uint32_t PUPDR;    /**< Pull-up/pull-down register */
    volatile uint32_t IDR;      /**< Input data register */
    volatile uint32_t ODR;      /**< Output data register */
    volatile uint32_t BSRR;     /**< Bit set/reset register */
    volatile uint32_t LCKR;     /**< Lock register */
    volatile uint32_t AFR[2];   /**< Alternate function registers */
} GPIO_TypeDef;

#define GPIOA   ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB   ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC   ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOG   ((GPIO_TypeDef *)GPIOG_BASE)

/**
 * @brief Registrador SYSCFG PMC (para seleção RMII/MII).
 */
#define SYSCFG_PMC          (*(volatile uint32_t *)(SYSCFG_BASE + 0x04))
#define SYSCFG_PMC_MII_RMII_SEL (1U << 23)

/**
 * @brief Valores para modo de GPIO.
 */
#define GPIO_MODE_INPUT     0x00
#define GPIO_MODE_OUTPUT    0x01
#define GPIO_MODE_AF        0x02
#define GPIO_MODE_ANALOG    0x03

/**
 * @brief Valores para velocidade de GPIO.
 */
#define GPIO_SPEED_LOW      0x00
#define GPIO_SPEED_MEDIUM   0x01
#define GPIO_SPEED_FAST     0x02
#define GPIO_SPEED_HIGH     0x03

/**
 * @brief Alternate function para Ethernet.
 */
#define GPIO_AF11_ETH       0x0B

/*-----------------------------------------------------------------------------
 * Funções Auxiliares
 *----------------------------------------------------------------------------*/

/**
 * @brief Configura um pino GPIO para função alternativa Ethernet.
 *
 * @param gpio Ponteiro para o periférico GPIO.
 * @param pin Número do pino (0-15).
 * @param af Função alternativa (GPIO_AF11_ETH para Ethernet).
 */
static void configureGpioEthernet(GPIO_TypeDef *gpio, uint8_t pin, uint8_t af)
{
    uint32_t temp;

    /* Configurar modo como função alternativa */
    temp = gpio->MODER;
    temp &= ~(0x03UL << (pin * 2));
    temp |= (GPIO_MODE_AF << (pin * 2));
    gpio->MODER = temp;

    /* Configurar como push-pull */
    gpio->OTYPER &= ~(1UL << pin);

    /* Configurar velocidade alta */
    temp = gpio->OSPEEDR;
    temp &= ~(0x03UL << (pin * 2));
    temp |= (GPIO_SPEED_HIGH << (pin * 2));
    gpio->OSPEEDR = temp;

    /* Sem pull-up/pull-down */
    temp = gpio->PUPDR;
    temp &= ~(0x03UL << (pin * 2));
    gpio->PUPDR = temp;

    /* Configurar função alternativa */
    if (pin < 8)
    {
        temp = gpio->AFR[0];
        temp &= ~(0x0FUL << (pin * 4));
        temp |= ((uint32_t)af << (pin * 4));
        gpio->AFR[0] = temp;
    }
    else
    {
        temp = gpio->AFR[1];
        temp &= ~(0x0FUL << ((pin - 8) * 4));
        temp |= ((uint32_t)af << ((pin - 8) * 4));
        gpio->AFR[1] = temp;
    }
}

/*-----------------------------------------------------------------------------
 * Funções Públicas
 *----------------------------------------------------------------------------*/

/**
 * @brief Inicializa os clocks e GPIOs para o controlador Ethernet.
 *
 * Configura os pinos para interface RMII conforme o layout do
 * STM32F429ZI-NUCLEO:
 *
 * | Sinal RMII     | Pino  |
 * |----------------|-------|
 * | ETH_RMII_REF_CLK | PA1   |
 * | ETH_MDIO       | PA2   |
 * | ETH_RMII_CRS_DV| PA7   |
 * | ETH_RMII_TX_EN | PG11  |
 * | ETH_RMII_TXD0  | PG13  |
 * | ETH_RMII_TXD1  | PB13  |
 * | ETH_RMII_RXD0  | PC4   |
 * | ETH_RMII_RXD1  | PC5   |
 * | ETH_MDC        | PC1   |
 *
 * @note Esta função deve ser chamada antes de inicializar o LwIP.
 */
void ethHardwareInit(void)
{
    /* Habilitar clock do SYSCFG */
    RCC_APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* Habilitar clocks dos GPIOs */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                   RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOGEN;

    /* Aguardar estabilização dos clocks */
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");

    /* Selecionar interface RMII */
    SYSCFG_PMC |= SYSCFG_PMC_MII_RMII_SEL;

    /* Configurar pinos GPIOA */
    configureGpioEthernet(GPIOA, 1, GPIO_AF11_ETH);  /* RMII_REF_CLK */
    configureGpioEthernet(GPIOA, 2, GPIO_AF11_ETH);  /* MDIO */
    configureGpioEthernet(GPIOA, 7, GPIO_AF11_ETH);  /* RMII_CRS_DV */

    /* Configurar pinos GPIOB */
    configureGpioEthernet(GPIOB, 13, GPIO_AF11_ETH); /* RMII_TXD1 */

    /* Configurar pinos GPIOC */
    configureGpioEthernet(GPIOC, 1, GPIO_AF11_ETH);  /* MDC */
    configureGpioEthernet(GPIOC, 4, GPIO_AF11_ETH);  /* RMII_RXD0 */
    configureGpioEthernet(GPIOC, 5, GPIO_AF11_ETH);  /* RMII_RXD1 */

    /* Configurar pinos GPIOG */
    configureGpioEthernet(GPIOG, 11, GPIO_AF11_ETH); /* RMII_TX_EN */
    configureGpioEthernet(GPIOG, 13, GPIO_AF11_ETH); /* RMII_TXD0 */

    /* Habilitar clocks do Ethernet MAC */
    RCC_AHB1ENR |= RCC_AHB1ENR_ETHMACEN | RCC_AHB1ENR_ETHMACTXEN |
                   RCC_AHB1ENR_ETHMACRXEN;

    /* Reset do Ethernet MAC */
    RCC_AHB1RSTR |= RCC_AHB1RSTR_ETHMACRST;
    __asm volatile("nop");
    __asm volatile("nop");
    RCC_AHB1RSTR &= ~RCC_AHB1RSTR_ETHMACRST;
}

/*-----------------------------------------------------------------------------
 * Funções de Acesso MDIO
 *----------------------------------------------------------------------------*/

/**
 * @brief Lê um registrador do PHY via interface MDIO.
 *
 * @param phyAddr Endereço do PHY no barramento (0-31).
 * @param regAddr Endereço do registrador no PHY (0-31).
 * @param value Ponteiro para armazenar o valor lido.
 * @return 0 se sucesso, -1 se timeout.
 */
static int ethPhyReadRegister(uint16_t phyAddr, uint16_t regAddr, uint16_t *value)
{
    uint32_t timeout = PHY_TIMEOUT;

    /* Aguardar MII não estar ocupado */
    while ((ETH_MACMIIAR & ETH_MACMIIAR_MB) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        return -1;
    }

    /* Configurar endereço do PHY e registrador para leitura */
    ETH_MACMIIAR = ETH_MACMIIAR_CR_HCLK_DIV42 |
                   ((uint32_t)phyAddr << ETH_MACMIIAR_PA_Pos) |
                   ((uint32_t)regAddr << ETH_MACMIIAR_MR_Pos) |
                   ETH_MACMIIAR_MB;  /* Sem MW = operação de leitura */

    /* Aguardar conclusão da operação */
    timeout = PHY_TIMEOUT;
    while ((ETH_MACMIIAR & ETH_MACMIIAR_MB) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        return -1;
    }

    /* Ler valor */
    *value = (uint16_t)(ETH_MACMIIDR & 0xFFFF);

    return 0;
}

/**
 * @brief Escreve em um registrador do PHY via interface MDIO.
 *
 * @param phyAddr Endereço do PHY no barramento (0-31).
 * @param regAddr Endereço do registrador no PHY (0-31).
 * @param value Valor a ser escrito.
 * @return 0 se sucesso, -1 se timeout.
 */
static int ethPhyWriteRegister(uint16_t phyAddr, uint16_t regAddr, uint16_t value)
{
    uint32_t timeout = PHY_TIMEOUT;

    /* Aguardar MII não estar ocupado */
    while ((ETH_MACMIIAR & ETH_MACMIIAR_MB) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        return -1;
    }

    /* Escrever valor no registrador de dados */
    ETH_MACMIIDR = (uint32_t)value;

    /* Configurar endereço do PHY e registrador para escrita */
    ETH_MACMIIAR = ETH_MACMIIAR_CR_HCLK_DIV42 |
                   ((uint32_t)phyAddr << ETH_MACMIIAR_PA_Pos) |
                   ((uint32_t)regAddr << ETH_MACMIIAR_MR_Pos) |
                   ETH_MACMIIAR_MW |  /* MW = operação de escrita */
                   ETH_MACMIIAR_MB;

    /* Aguardar conclusão da operação */
    timeout = PHY_TIMEOUT;
    while ((ETH_MACMIIAR & ETH_MACMIIAR_MB) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Inicializa o PHY Ethernet (LAN8742A).
 *
 * Configura o PHY externo via interface MDIO. O LAN8742A suporta
 * auto-negociação e pode operar em 10/100 Mbps.
 *
 * @return 0 se sucesso, -1 se erro.
 */
int ethPhyInit(void)
{
    uint16_t phyReg;
    uint32_t timeout;

    printf("[ETH] Inicializando PHY LAN8742A...\n");

    /* 1. Verificar ID do PHY */
    if (ethPhyReadRegister(PHY_ADDRESS, PHY_IDR1, &phyReg) != 0)
    {
        printf("[ETH] ERRO: Falha ao ler ID1 do PHY\n");
        return -1;
    }
    printf("[ETH] PHY ID1: 0x%04X\n", phyReg);

    if (phyReg != LAN8742A_PHY_ID1)
    {
        printf("[ETH] AVISO: ID1 inesperado (esperado 0x%04X)\n", LAN8742A_PHY_ID1);
        /* Continuar mesmo assim, pode ser outro PHY compatível */
    }

    if (ethPhyReadRegister(PHY_ADDRESS, PHY_IDR2, &phyReg) != 0)
    {
        printf("[ETH] ERRO: Falha ao ler ID2 do PHY\n");
        return -1;
    }
    printf("[ETH] PHY ID2: 0x%04X\n", phyReg);

    /* 2. Soft reset do PHY */
    printf("[ETH] Executando soft reset do PHY...\n");
    if (ethPhyWriteRegister(PHY_ADDRESS, PHY_BCR, PHY_BCR_SOFT_RESET) != 0)
    {
        printf("[ETH] ERRO: Falha ao resetar PHY\n");
        return -1;
    }

    /* Aguardar reset completar (bit auto-limpa) */
    timeout = PHY_TIMEOUT;
    do
    {
        if (ethPhyReadRegister(PHY_ADDRESS, PHY_BCR, &phyReg) != 0)
        {
            printf("[ETH] ERRO: Falha ao ler BCR após reset\n");
            return -1;
        }
        timeout--;
    } while ((phyReg & PHY_BCR_SOFT_RESET) && (timeout > 0));

    if (timeout == 0)
    {
        printf("[ETH] ERRO: Timeout aguardando reset do PHY\n");
        return -1;
    }
    printf("[ETH] Reset do PHY concluído\n");

    /* 3. Habilitar auto-negociação */
    printf("[ETH] Iniciando auto-negociacao...\n");
    if (ethPhyWriteRegister(PHY_ADDRESS, PHY_BCR,
                            PHY_BCR_AUTONEG_EN | PHY_BCR_AUTONEG_RESTART) != 0)
    {
        printf("[ETH] ERRO: Falha ao iniciar auto-negociacao\n");
        return -1;
    }

    /* 4. Aguardar conclusão da auto-negociação (com timeout de ~5 segundos) */
    timeout = 5000000;  /* Aumentado para dar tempo de negociação */
    do
    {
        if (ethPhyReadRegister(PHY_ADDRESS, PHY_BSR, &phyReg) != 0)
        {
            printf("[ETH] ERRO: Falha ao ler BSR\n");
            return -1;
        }

        /* Verificar se link está ativo */
        if ((phyReg & PHY_BSR_LINK_STATUS) && (phyReg & PHY_BSR_AUTONEG_COMPLETE))
        {
            break;
        }

        timeout--;
    } while (timeout > 0);

    if (timeout == 0)
    {
        printf("[ETH] AVISO: Timeout na auto-negociacao (link pode estar desconectado)\n");
        /* Não retornar erro, o link pode ser estabelecido depois */
    }

    /* 5. Verificar estado do link */
    if (ethPhyReadRegister(PHY_ADDRESS, PHY_BSR, &phyReg) != 0)
    {
        printf("[ETH] ERRO: Falha ao ler status do link\n");
        return -1;
    }

    if (phyReg & PHY_BSR_LINK_STATUS)
    {
        printf("[ETH] Link detectado!\n");

        /* Ler velocidade e modo duplex do registrador específico do LAN8742A */
        if (ethPhyReadRegister(PHY_ADDRESS, PHY_SPECIAL_CTRL_STATUS, &phyReg) == 0)
        {
            uint32_t macCr = ETH_MACCR;

            switch (phyReg & PHY_SCSR_SPEED_MASK)
            {
                case PHY_SCSR_100FD:
                    printf("[ETH] Velocidade: 100 Mbps Full Duplex\n");
                    macCr |= (1U << 14);  /* FES = 100Mbps */
                    macCr |= (1U << 11);  /* DM = Full Duplex */
                    break;
                case PHY_SCSR_100HD:
                    printf("[ETH] Velocidade: 100 Mbps Half Duplex\n");
                    macCr |= (1U << 14);  /* FES = 100Mbps */
                    macCr &= ~(1U << 11); /* DM = Half Duplex */
                    break;
                case PHY_SCSR_10FD:
                    printf("[ETH] Velocidade: 10 Mbps Full Duplex\n");
                    macCr &= ~(1U << 14); /* FES = 10Mbps */
                    macCr |= (1U << 11);  /* DM = Full Duplex */
                    break;
                case PHY_SCSR_10HD:
                    printf("[ETH] Velocidade: 10 Mbps Half Duplex\n");
                    macCr &= ~(1U << 14); /* FES = 10Mbps */
                    macCr &= ~(1U << 11); /* DM = Half Duplex */
                    break;
                default:
                    printf("[ETH] Velocidade desconhecida: 0x%02X\n", phyReg & PHY_SCSR_SPEED_MASK);
                    break;
            }

            ETH_MACCR = macCr;
        }
    }
    else
    {
        printf("[ETH] AVISO: Link nao detectado (cabo desconectado?)\n");
    }

    printf("[ETH] Inicializacao do PHY concluida\n");
    return 0;
}

/**
 * @brief Verifica o estado atual do link Ethernet.
 *
 * @return 1 se link ativo, 0 se inativo, -1 se erro.
 */
int ethPhyGetLinkStatus(void)
{
    uint16_t phyReg;

    if (ethPhyReadRegister(PHY_ADDRESS, PHY_BSR, &phyReg) != 0)
    {
        return -1;
    }

    return (phyReg & PHY_BSR_LINK_STATUS) ? 1 : 0;
}
