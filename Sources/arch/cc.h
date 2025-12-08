/**
 * @file cc.h
 * @brief Definições de compilador e arquitetura para LwIP no STM32.
 *
 * Este arquivo define tipos, macros e configurações específicas do compilador
 * GCC ARM para uso com a pilha LwIP em microcontroladores STM32.
 */

#ifndef CC_H
#define CC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

/* Incluir CMSIS para funções de controle de interrupção */
#include "stm32f4xx.h"

/*-----------------------------------------------------------------------------
 * Definições de Tipos Básicos
 *----------------------------------------------------------------------------*/

/**
 * @brief Tipos inteiros sem sinal.
 */
typedef uint8_t   u8_t;
typedef uint16_t  u16_t;
typedef uint32_t  u32_t;

/**
 * @brief Tipos inteiros com sinal.
 */
typedef int8_t    s8_t;
typedef int16_t   s16_t;
typedef int32_t   s32_t;

/**
 * @brief Tipo para ponteiros de memória.
 */
typedef uintptr_t mem_ptr_t;

/*-----------------------------------------------------------------------------
 * Configurações de Alinhamento e Empacotamento
 *----------------------------------------------------------------------------*/

/**
 * @brief Macro para empacotar estruturas (sem padding).
 */
#define PACK_STRUCT_FIELD(x)    x
#define PACK_STRUCT_STRUCT      __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

/*-----------------------------------------------------------------------------
 * Configurações de Ordem de Bytes (Endianness)
 *----------------------------------------------------------------------------*/

/**
 * @brief Define a ordem de bytes como little-endian (ARM Cortex-M).
 */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

/*-----------------------------------------------------------------------------
 * Macros de Diagnóstico e Debug
 *----------------------------------------------------------------------------*/

/**
 * @brief Macro para imprimir mensagens de diagnóstico.
 *        Usa printf padrão, pode ser redirecionada para UART.
 */
#ifndef LWIP_PLATFORM_DIAG
#define LWIP_PLATFORM_DIAG(x)   do { printf x; } while(0)
#endif

/**
 * @brief Macro para asserções do LwIP.
 *        Em caso de falha, entra em loop infinito.
 */
#ifndef LWIP_PLATFORM_ASSERT
#define LWIP_PLATFORM_ASSERT(x) do { \
    printf("Assertion \"%s\" failed at line %d in %s\n", \
           x, __LINE__, __FILE__); \
    while(1); \
} while(0)
#endif

/*-----------------------------------------------------------------------------
 * Macros de Formatação para printf
 *----------------------------------------------------------------------------*/

/**
 * @brief Formatos para tipos LwIP em printf.
 */
#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "lu"
#define S32_F "ld"
#define X32_F "lx"
#define SZT_F "zu"

/*-----------------------------------------------------------------------------
 * Gerador de Números Aleatórios
 *----------------------------------------------------------------------------*/

/**
 * @brief Macro para geração de números aleatórios.
 *        Usado pelo LwIP para DNS, TCP ISN, etc.
 *        Implementação simples usando SysTick.
 */
extern uint32_t lwip_rand(void);
#define LWIP_RAND() lwip_rand()

/*-----------------------------------------------------------------------------
 * Proteção de Seção Crítica
 *----------------------------------------------------------------------------*/

/**
 * @brief Tipo para armazenar estado de interrupção.
 */
#define SYS_ARCH_DECL_PROTECT(lev) uint32_t lev

/**
 * @brief Desabilita interrupções e salva estado.
 */
#define SYS_ARCH_PROTECT(lev) do { \
    lev = __get_PRIMASK(); \
    __disable_irq(); \
} while(0)

/**
 * @brief Restaura estado de interrupção.
 */
#define SYS_ARCH_UNPROTECT(lev) do { \
    __set_PRIMASK(lev); \
} while(0)

/*-----------------------------------------------------------------------------
 * Funções CMSIS para Controle de Interrupções
 *
 * NOTA: Estas funções são definidas pelo CMSIS (cmsis_gcc.h) quando USE_HAL_DRIVER
 * está habilitado. Não precisamos redefini-las.
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* CC_H */
