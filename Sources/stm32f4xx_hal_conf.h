/**
 * @file stm32f4xx_hal_conf.h
 * @brief Arquivo de configuração do STM32 HAL para STM32F429ZI.
 *
 * Este arquivo define quais módulos do HAL serão utilizados e seus parâmetros.
 * Baseado no template do STM32CubeF4.
 */

#ifndef STM32F4XX_HAL_CONF_H
#define STM32F4XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------
 * Seleção de Módulos HAL
 *----------------------------------------------------------------------------*/

/**
 * @brief Módulos HAL habilitados.
 *        Comente as linhas dos módulos não utilizados para economizar espaço.
 */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_ETH_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED

/* Módulos não utilizados (comentados para referência) */
/* #define HAL_CAN_MODULE_ENABLED */
/* #define HAL_CRC_MODULE_ENABLED */
/* #define HAL_DAC_MODULE_ENABLED */
/* #define HAL_DCMI_MODULE_ENABLED */
/* #define HAL_DMA2D_MODULE_ENABLED */
/* #define HAL_FLASH_MODULE_ENABLED */
/* #define HAL_I2C_MODULE_ENABLED */
/* #define HAL_I2S_MODULE_ENABLED */
/* #define HAL_IWDG_MODULE_ENABLED */
/* #define HAL_LTDC_MODULE_ENABLED */
/* #define HAL_RNG_MODULE_ENABLED */
/* #define HAL_RTC_MODULE_ENABLED */
/* #define HAL_SAI_MODULE_ENABLED */
/* #define HAL_SD_MODULE_ENABLED */
/* #define HAL_SPI_MODULE_ENABLED */
/* #define HAL_TIM_MODULE_ENABLED */
/* #define HAL_WWDG_MODULE_ENABLED */

/*-----------------------------------------------------------------------------
 * Configuração do Oscilador
 *----------------------------------------------------------------------------*/

/**
 * @brief Frequência do oscilador HSE externo em Hz.
 *        O NUCLEO-F429ZI usa um cristal de 8 MHz.
 */
#if !defined(HSE_VALUE)
    #define HSE_VALUE    ((uint32_t)8000000U)
#endif

/**
 * @brief Timeout para estabilização do HSE (em ms).
 */
#if !defined(HSE_STARTUP_TIMEOUT)
    #define HSE_STARTUP_TIMEOUT    ((uint32_t)100U)
#endif

/**
 * @brief Frequência do oscilador HSI interno em Hz.
 */
#if !defined(HSI_VALUE)
    #define HSI_VALUE    ((uint32_t)16000000U)
#endif

/**
 * @brief Frequência do oscilador LSI interno em Hz.
 */
#if !defined(LSI_VALUE)
    #define LSI_VALUE    ((uint32_t)32000U)
#endif

/**
 * @brief Frequência do oscilador LSE externo em Hz.
 */
#if !defined(LSE_VALUE)
    #define LSE_VALUE    ((uint32_t)32768U)
#endif

/**
 * @brief Timeout para estabilização do LSE (em ms).
 */
#if !defined(LSE_STARTUP_TIMEOUT)
    #define LSE_STARTUP_TIMEOUT    ((uint32_t)5000U)
#endif

/**
 * @brief Frequência de entrada externa para I2S em Hz.
 */
#if !defined(EXTERNAL_CLOCK_VALUE)
    #define EXTERNAL_CLOCK_VALUE    ((uint32_t)12288000U)
#endif

/*-----------------------------------------------------------------------------
 * Configuração do Sistema
 *----------------------------------------------------------------------------*/

/**
 * @brief Valor de prioridade de interrupção para o SysTick.
 *        Deve ser maior (menor prioridade) que as interrupções do FreeRTOS.
 */
#define TICK_INT_PRIORITY           ((uint32_t)15U)

/**
 * @brief Usar RTOS para gerenciar o tick do sistema.
 *        NOTA: O HAL da ST não suporta USE_RTOS=1 diretamente.
 *        O tick é gerenciado manualmente em system_config.c.
 */
#define USE_RTOS                    0

/**
 * @brief Habilitar callbacks de prefetch (não usado).
 */
#define PREFETCH_ENABLE             1
#define INSTRUCTION_CACHE_ENABLE    1
#define DATA_CACHE_ENABLE           1

/*-----------------------------------------------------------------------------
 * Configuração do Ethernet
 *----------------------------------------------------------------------------*/

/**
 * @brief Endereço do PHY no barramento MDIO.
 *        LAN8742A no NUCLEO-F429ZI usa endereço 0.
 */
#define LAN8742A_PHY_ADDRESS        0x00U

/**
 * @brief Timeout para operações de leitura/escrita do PHY.
 */
#define PHY_READ_TO                 ((uint32_t)0x0000FFFFU)
#define PHY_WRITE_TO                ((uint32_t)0x0000FFFFU)

/**
 * @brief Definições do PHY LAN8742A para o HAL ETH.
 */
#define PHY_BCR                     ((uint16_t)0x00U)   /**< Basic Control Register */
#define PHY_BSR                     ((uint16_t)0x01U)   /**< Basic Status Register */

#define PHY_RESET                   ((uint16_t)0x8000U) /**< PHY Reset */
#define PHY_LOOPBACK                ((uint16_t)0x4000U) /**< Loopback mode */
#define PHY_FULLDUPLEX_100M         ((uint16_t)0x2100U) /**< 100Mbps Full-duplex */
#define PHY_HALFDUPLEX_100M         ((uint16_t)0x2000U) /**< 100Mbps Half-duplex */
#define PHY_FULLDUPLEX_10M          ((uint16_t)0x0100U) /**< 10Mbps Full-duplex */
#define PHY_HALFDUPLEX_10M          ((uint16_t)0x0000U) /**< 10Mbps Half-duplex */
#define PHY_AUTONEGOTIATION         ((uint16_t)0x1000U) /**< Auto-negotiation enable */
#define PHY_RESTART_AUTONEGOTIATION ((uint16_t)0x0200U) /**< Restart auto-negotiation */
#define PHY_POWERDOWN               ((uint16_t)0x0800U) /**< Power down mode */
#define PHY_ISOLATE                 ((uint16_t)0x0400U) /**< Isolate PHY */

#define PHY_AUTONEGO_COMPLETE       ((uint16_t)0x0020U) /**< Auto-negotiation complete */
#define PHY_LINKED_STATUS           ((uint16_t)0x0004U) /**< Link status */
#define PHY_JABBER_DETECTION        ((uint16_t)0x0002U) /**< Jabber detection */

/**
 * @brief Registrador especial de status do LAN8742A (reg 31).
 */
#define PHY_SR                      ((uint16_t)0x1FU)   /**< Special Status Register */
#define PHY_SPEED_STATUS            ((uint16_t)0x0004U) /**< Speed status (1=10Mbps) */
#define PHY_DUPLEX_STATUS           ((uint16_t)0x0010U) /**< Duplex status (1=Full) */

/**
 * @brief Delay de configuração do PHY.
 */
#define PHY_CONFIG_DELAY            ((uint32_t)500U)    /**< Delay em ms */
#define PHY_RESET_DELAY             ((uint32_t)500U)    /**< Delay de reset */

/**
 * @brief Número de descritores DMA para RX e TX.
 */
#define ETH_RXBUFNB                 ((uint32_t)4U)
#define ETH_TXBUFNB                 ((uint32_t)4U)

/**
 * @brief Endereço MAC padrão.
 */
#define ETH_MAC_ADDR0               ((uint8_t)0x02U)
#define ETH_MAC_ADDR1               ((uint8_t)0x00U)
#define ETH_MAC_ADDR2               ((uint8_t)0x00U)
#define ETH_MAC_ADDR3               ((uint8_t)0x00U)
#define ETH_MAC_ADDR4               ((uint8_t)0x00U)
#define ETH_MAC_ADDR5               ((uint8_t)0x01U)

/*-----------------------------------------------------------------------------
 * Configuração de Asserção e Debug
 *----------------------------------------------------------------------------*/

/**
 * @brief Macro de asserção do HAL.
 *        Descomente para habilitar verificação de parâmetros.
 */
/* #define USE_FULL_ASSERT    1U */

#ifdef USE_FULL_ASSERT
    #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
    void assert_failed(uint8_t *file, uint32_t line);
#else
    #define assert_param(expr) ((void)0U)
#endif

/*-----------------------------------------------------------------------------
 * Includes dos Módulos HAL
 *----------------------------------------------------------------------------*/

#ifdef HAL_RCC_MODULE_ENABLED
    #include "stm32f4xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
    #include "stm32f4xx_hal_gpio.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
    #include "stm32f4xx_hal_dma.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
    #include "stm32f4xx_hal_cortex.h"
#endif

#ifdef HAL_ETH_MODULE_ENABLED
    #include "stm32f4xx_hal_eth.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
    #include "stm32f4xx_hal_flash.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
    #include "stm32f4xx_hal_pwr.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
    #include "stm32f4xx_hal_uart.h"
#endif

#ifdef HAL_ADC_MODULE_ENABLED
    #include "stm32f4xx_hal_adc.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* STM32F4XX_HAL_CONF_H */
