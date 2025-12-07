/**
 * @file system_config.h
 * @brief Interface pública para configuração do sistema STM32F429ZI.
 *
 * Este arquivo declara as funções de configuração de clocks e UART.
 */

#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------
 * Definições de Clock
 *----------------------------------------------------------------------------*/

/**
 * @brief Frequência do clock do sistema (SYSCLK) em Hz.
 *        180 MHz - máximo do STM32F429ZI.
 */
#define SYSCLK_FREQ_HZ      180000000UL

/**
 * @brief Frequência do barramento AHB em Hz.
 */
#define HCLK_FREQ_HZ        180000000UL

/**
 * @brief Frequência do barramento APB1 em Hz.
 *        Máximo 45 MHz (180/4).
 */
#define APB1_FREQ_HZ        45000000UL

/**
 * @brief Frequência do barramento APB2 em Hz.
 *        Máximo 90 MHz (180/2).
 */
#define APB2_FREQ_HZ        90000000UL

/*-----------------------------------------------------------------------------
 * Funções Públicas
 *----------------------------------------------------------------------------*/

/**
 * @brief Inicialização mínima do sistema chamada pelo startup.
 *
 * Esta função é chamada pelo Reset_Handler ANTES de:
 * - Copiar a seção .data da Flash para RAM
 * - Zerar a seção .bss
 * - Chamar construtores estáticos
 * - Chamar main()
 *
 * Por isso, NÃO pode usar variáveis globais ou estáticas inicializadas.
 *
 * Responsabilidades:
 * - Habilitar a FPU (Floating Point Unit)
 * - Configurar o VTOR (Vector Table Offset Register)
 * - Configurar clock básico se necessário
 *
 * @warning Esta função NÃO pode usar variáveis globais inicializadas!
 */
void SystemInit(void);

/**
 * @brief Configura o sistema de clocks.
 *
 * Configura o PLL usando HSE (8 MHz) para gerar:
 * - SYSCLK = 82 MHz
 * - AHB = 82 MHz (HCLK)
 * - APB1 = 41 MHz
 * - APB2 = 82 MHz
 *
 * @note Esta função é chamada automaticamente pelo SystemInit().
 *       Não é necessário chamá-la manualmente em main().
 *
 * @note A placa NUCLEO-F429ZI possui um cristal HSE de 8 MHz.
 *
 * @note Esta função NÃO usa variáveis globais, por isso pode ser
 *       chamada antes do C runtime estar inicializado.
 */
void systemClockConfig(void);

/**
 * @brief Inicializa a UART2 para comunicação serial.
 *
 * Configuração:
 * - Pinos: PA2 (TX), PA3 (RX)
 * - Baud rate: 115200
 * - Formato: 8N1 (8 bits, sem paridade, 1 stop bit)
 *
 * @note A UART2 é usada para redirecionamento do printf.
 *
 * @note Na placa NUCLEO-F429ZI, PA2 e PA3 estão disponíveis
 *       no conector CN10 (Arduino-compatible header).
 */
void uart3Init(void);

/**
 * @brief Envia um caractere pela UART2.
 *
 * @param ch Caractere a enviar.
 */
void uart3SendChar(char ch);

/**
 * @brief Recebe um caractere da UART2 (bloqueante).
 *
 * @return Caractere recebido.
 *
 * @note Esta função bloqueia até um caractere ser recebido.
 */
char uart3ReceiveChar(void);

/**
 * @brief Envia uma string pela UART2.
 *
 * @param str String terminada em null ('\0').
 */
void uart3SendString(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_CONFIG_H */
