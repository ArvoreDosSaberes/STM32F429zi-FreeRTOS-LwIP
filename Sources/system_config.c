/**
 * @file system_config.c
 * @brief Configuração do sistema de clocks e periféricos para STM32F429ZI.
 *
 * Este arquivo configura:
 * - Sistema de clocks: SYSCLK = 180 MHz (usando HAL)
 * - UART3 para saída do printf (PA8=TX, PA9=RX)
 *
 * @note O STM32F429ZI suporta até 180 MHz.
 */

#include <stdint.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "system_config.h"

#include "FreeRTOS.h"
#include "task.h"

/*-----------------------------------------------------------------------------
 * Implementação das Funções
 *----------------------------------------------------------------------------*/

/* NOTA: SystemInit() é fornecido pelo system_stm32f4xx.c do CMSIS.
 * Ele configura FPU e VTOR automaticamente.
 */

/**
 * @brief Configura o sistema de clocks usando HAL.
 *
 * Configura o PLL usando HSE (8 MHz) para gerar:
 * - SYSCLK = 180 MHz (máximo do STM32F429)
 * - AHB = 180 MHz
 * - APB1 = 45 MHz
 * - APB2 = 90 MHz
 */
void systemClockConfig(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Configurar regulador de tensão para máximo desempenho */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Configurar HSE e PLL
     * NOTA: Na NUCLEO-F429ZI, o HSE vem do ST-LINK em modo bypass
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 360;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        /* Erro na configuração do oscilador */
        while (1) {}
    }

    /* Habilitar Over-drive para 180 MHz */
    if (HAL_PWREx_EnableOverDrive() != HAL_OK)
    {
        /* Erro no over-drive */
        while (1) {}
    }

    /* Configurar clocks do sistema */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    /* Flash latency = 5 wait states para 180 MHz */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        /* Erro na configuração de clock */
        while (1) {}
    }

    /* Atualizar SystemCoreClock */
    SystemCoreClockUpdate();
}

/* Handle da UART3 para uso com HAL */
static UART_HandleTypeDef huart3;

/**
 * @brief Inicializa a UART3 para comunicação serial (ST-LINK VCP).
 *
 * Configuração:
 * - Pinos: PD8 (TX), PD9 (RX) - conectados ao ST-LINK VCP na NUCLEO-F429ZI
 * - Baud rate: 115200
 * - 8 bits de dados, sem paridade, 1 stop bit
 *
 * @note A UART3 está no barramento APB1 (45 MHz).
 */
void uart3Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* Habilitar clocks */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    
    /* Configurar PD8 (TX) e PD9 (RX) como função alternativa */
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    
    /* Configurar UART3 */
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&huart3) != HAL_OK)
    {
        /* Erro na inicialização - piscar LED rapidamente */
        __HAL_RCC_GPIOB_CLK_ENABLE();
        GPIOB->MODER = (GPIOB->MODER & ~(3U << 0)) | (1U << 0);
        while (1)
        {
            GPIOB->ODR ^= (1U << 0);
            for (volatile int d = 0; d < 100000; d++) {}
        }
    }
    
}

/**
 * @brief Envia um caractere pela UART3.
 *
 * @param ch Caractere a enviar.
 */
void uart3SendChar(char ch)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 100);
}

/**
 * @brief Recebe um caractere da UART3 (bloqueante).
 *
 * @return Caractere recebido.
 */
char uart3ReceiveChar(void)
{
    uint8_t ch = 0;
    HAL_UART_Receive(&huart3, &ch, 1, HAL_MAX_DELAY);
    return (char)ch;
}

/**
 * @brief Envia uma string pela UART3.
 *
 * @param str String terminada em null.
 */
void uart3SendString(const char *str)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)str, strlen(str), HAL_MAX_DELAY);
}

/*-----------------------------------------------------------------------------
 * Nota: HAL_GetTick, HAL_IncTick e HAL_Delay são fornecidos pelo
 * stm32f4xx_hal.c do STM32CubeF4.
 *----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 * Handler do SysTick
 *----------------------------------------------------------------------------*/

/* Declaração externa do handler do FreeRTOS */
extern void xPortSysTickHandler(void);

/**
 * @brief Handler do SysTick.
 *
 * Este handler funciona tanto antes quanto depois do scheduler do FreeRTOS
 * iniciar. Sempre incrementa o tick do HAL, e quando o scheduler está
 * rodando, também chama o handler do FreeRTOS.
 */
void SysTick_Handler(void)
{
    /* Sempre incrementar o tick do HAL */
    HAL_IncTick();

    /* Se o scheduler do FreeRTOS está rodando, chamar seu handler */
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
}

/*-----------------------------------------------------------------------------
 * Handlers de Fault para Debug
 *----------------------------------------------------------------------------*/

/**
 * @brief Handler de HardFault para debug.
 *
 * Envia informação de diagnóstico pela UART antes de travar.
 */
void HardFault_Handler(void)
{
    uart3SendString("\r\n!!! HARD FAULT !!!\r\n");
    
    /* Loop infinito para debug */
    __asm volatile("bkpt #0");
    while (1) {}
}

/**
 * @brief Handler de MemManage para debug.
 */
void MemManage_Handler(void)
{
    uart3SendString("\r\n!!! MEM MANAGE FAULT !!!\r\n");
    while (1) {}
}

/**
 * @brief Handler de BusFault para debug.
 */
void BusFault_Handler(void)
{
    uart3SendString("\r\n!!! BUS FAULT !!!\r\n");
    while (1) {}
}

/**
 * @brief Handler de UsageFault para debug.
 */
void UsageFault_Handler(void)
{
    uart3SendString("\r\n!!! USAGE FAULT !!!\r\n");
    while (1) {}
}

/**
 * @brief Função de saída de caractere para printf (newlib).
 *
 * Esta função é chamada pelo _write() do syscalls.c.
 *
 * @param ch Caractere a enviar.
 * @return O caractere enviado.
 */
int __io_putchar(int ch)
{
    uart3SendChar((char)ch);
    return ch;
}

/**
 * @brief Função de entrada de caractere para scanf (newlib).
 *
 * Esta função é chamada pelo _read() do syscalls.c.
 *
 * @return O caractere recebido.
 */
int __io_getchar(void)
{
    return (int)uart3ReceiveChar();
}
