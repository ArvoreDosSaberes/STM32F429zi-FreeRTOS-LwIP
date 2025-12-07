/**
 * @file sys_arch.h
 * @brief Definições de interface do sistema para LwIP com FreeRTOS.
 *
 * Este arquivo define os tipos e estruturas necessários para a integração
 * do LwIP com o sistema operacional FreeRTOS no STM32F429.
 */

#ifndef SYS_ARCH_H
#define SYS_ARCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*-----------------------------------------------------------------------------
 * Definições de Tipos para Sincronização
 *----------------------------------------------------------------------------*/

/**
 * @brief Tipo para semáforos do LwIP.
 *        Usa semáforos binários do FreeRTOS.
 */
typedef SemaphoreHandle_t sys_sem_t;

/**
 * @brief Tipo para mutex do LwIP.
 *        Usa mutex recursivo do FreeRTOS.
 */
typedef SemaphoreHandle_t sys_mutex_t;

/**
 * @brief Tipo para mailbox (fila de mensagens) do LwIP.
 *        Usa filas do FreeRTOS.
 */
typedef QueueHandle_t sys_mbox_t;

/**
 * @brief Tipo para threads do LwIP.
 *        Usa handles de task do FreeRTOS.
 */
typedef TaskHandle_t sys_thread_t;

/*-----------------------------------------------------------------------------
 * Macros para Verificação de Validade
 *----------------------------------------------------------------------------*/

/**
 * @brief Verifica se um semáforo é válido.
 * @param sem Semáforo a verificar.
 * @return 1 se válido, 0 se inválido.
 */
#define sys_sem_valid(sem)      ((sem) != NULL && *(sem) != NULL)

/**
 * @brief Define um semáforo como inválido.
 * @param sem Semáforo a invalidar.
 */
#define sys_sem_set_invalid(sem) do { if((sem) != NULL) { *(sem) = NULL; } } while(0)

/**
 * @brief Verifica se um mutex é válido.
 * @param mutex Mutex a verificar.
 * @return 1 se válido, 0 se inválido.
 */
#define sys_mutex_valid(mutex)  ((mutex) != NULL && *(mutex) != NULL)

/**
 * @brief Define um mutex como inválido.
 * @param mutex Mutex a invalidar.
 */
#define sys_mutex_set_invalid(mutex) do { if((mutex) != NULL) { *(mutex) = NULL; } } while(0)

/**
 * @brief Verifica se uma mailbox é válida.
 * @param mbox Mailbox a verificar.
 * @return 1 se válida, 0 se inválida.
 */
#define sys_mbox_valid(mbox)    ((mbox) != NULL && *(mbox) != NULL)

/**
 * @brief Define uma mailbox como inválida.
 * @param mbox Mailbox a invalidar.
 */
#define sys_mbox_set_invalid(mbox) do { if((mbox) != NULL) { *(mbox) = NULL; } } while(0)

#ifdef __cplusplus
}
#endif

#endif /* SYS_ARCH_H */
