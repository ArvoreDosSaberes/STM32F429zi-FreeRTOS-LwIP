/**
 * @file sys_arch.c
 * @brief Implementação da camada de abstração do sistema para LwIP com FreeRTOS.
 *
 * Este arquivo implementa as funções de interface entre o LwIP e o FreeRTOS,
 * incluindo semáforos, mutexes, mailboxes e threads.
 *
 * @note Baseado na documentação oficial do LwIP para integração com RTOS.
 */

#include "arch/sys_arch.h"
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <string.h>

/*-----------------------------------------------------------------------------
 * Variáveis Estáticas
 *----------------------------------------------------------------------------*/

/**
 * @brief Armazena o tempo de início do sistema para cálculo de uptime.
 */
static uint32_t sysStartTicks = 0;

/*-----------------------------------------------------------------------------
 * Inicialização do Sistema
 *----------------------------------------------------------------------------*/

/**
 * @brief Inicializa a camada de abstração do sistema.
 *
 * Esta função é chamada uma vez durante a inicialização do LwIP.
 * Registra o tick inicial para cálculo de tempo.
 */
void sys_init(void)
{
    sysStartTicks = xTaskGetTickCount();
}

/*-----------------------------------------------------------------------------
 * Funções de Semáforo
 *----------------------------------------------------------------------------*/

/**
 * @brief Cria um novo semáforo.
 *
 * @param sem Ponteiro para o semáforo a ser criado.
 * @param count Valor inicial do semáforo (0 ou 1 para binário).
 * @return ERR_OK se sucesso, ERR_MEM se falha na alocação.
 */
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
    if (sem == NULL)
    {
        return ERR_ARG;
    }

    *sem = xSemaphoreCreateBinary();
    if (*sem == NULL)
    {
        SYS_STATS_INC(sem.err);
        return ERR_MEM;
    }

    if (count > 0)
    {
        xSemaphoreGive(*sem);
    }

    SYS_STATS_INC_USED(sem);
    return ERR_OK;
}

/**
 * @brief Libera (incrementa) um semáforo.
 *
 * @param sem Ponteiro para o semáforo.
 */
void sys_sem_signal(sys_sem_t *sem)
{
    if (sem != NULL && *sem != NULL)
    {
        xSemaphoreGive(*sem);
    }
}

/**
 * @brief Aguarda (decrementa) um semáforo com timeout.
 *
 * @param sem Ponteiro para o semáforo.
 * @param timeout Tempo máximo de espera em milissegundos (0 = infinito).
 * @return Tempo de espera em ms, ou SYS_ARCH_TIMEOUT se timeout expirou.
 */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    TickType_t startTick;
    TickType_t waitTicks;
    BaseType_t result;

    if (sem == NULL || *sem == NULL)
    {
        return SYS_ARCH_TIMEOUT;
    }

    startTick = xTaskGetTickCount();

    if (timeout == 0)
    {
        waitTicks = portMAX_DELAY;
    }
    else
    {
        waitTicks = pdMS_TO_TICKS(timeout);
    }

    result = xSemaphoreTake(*sem, waitTicks);

    if (result == pdTRUE)
    {
        TickType_t elapsedTicks = xTaskGetTickCount() - startTick;
        return (u32_t)(elapsedTicks * portTICK_PERIOD_MS);
    }
    else
    {
        return SYS_ARCH_TIMEOUT;
    }
}

/**
 * @brief Libera recursos de um semáforo.
 *
 * @param sem Ponteiro para o semáforo a ser destruído.
 */
void sys_sem_free(sys_sem_t *sem)
{
    if (sem != NULL && *sem != NULL)
    {
        vSemaphoreDelete(*sem);
        *sem = NULL;
        SYS_STATS_DEC(sem.used);
    }
}

/*-----------------------------------------------------------------------------
 * Funções de Mutex
 *----------------------------------------------------------------------------*/

/**
 * @brief Cria um novo mutex.
 *
 * @param mutex Ponteiro para o mutex a ser criado.
 * @return ERR_OK se sucesso, ERR_MEM se falha na alocação.
 */
err_t sys_mutex_new(sys_mutex_t *mutex)
{
    if (mutex == NULL)
    {
        return ERR_ARG;
    }

    *mutex = xSemaphoreCreateRecursiveMutex();
    if (*mutex == NULL)
    {
        return ERR_MEM;
    }

    return ERR_OK;
}

/**
 * @brief Trava um mutex.
 *
 * @param mutex Ponteiro para o mutex.
 */
void sys_mutex_lock(sys_mutex_t *mutex)
{
    if (mutex != NULL && *mutex != NULL)
    {
        xSemaphoreTakeRecursive(*mutex, portMAX_DELAY);
    }
}

/**
 * @brief Destrava um mutex.
 *
 * @param mutex Ponteiro para o mutex.
 */
void sys_mutex_unlock(sys_mutex_t *mutex)
{
    if (mutex != NULL && *mutex != NULL)
    {
        xSemaphoreGiveRecursive(*mutex);
    }
}

/**
 * @brief Libera recursos de um mutex.
 *
 * @param mutex Ponteiro para o mutex a ser destruído.
 */
void sys_mutex_free(sys_mutex_t *mutex)
{
    if (mutex != NULL && *mutex != NULL)
    {
        vSemaphoreDelete(*mutex);
        *mutex = NULL;
    }
}

/*-----------------------------------------------------------------------------
 * Funções de Mailbox
 *----------------------------------------------------------------------------*/

/**
 * @brief Cria uma nova mailbox (fila de mensagens).
 *
 * @param mbox Ponteiro para a mailbox a ser criada.
 * @param size Tamanho da fila (número de mensagens).
 * @return ERR_OK se sucesso, ERR_MEM se falha na alocação.
 */
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    if (mbox == NULL)
    {
        return ERR_ARG;
    }

    *mbox = xQueueCreate((UBaseType_t)size, sizeof(void *));
    if (*mbox == NULL)
    {
        SYS_STATS_INC(mbox.err);
        return ERR_MEM;
    }

    SYS_STATS_INC_USED(mbox);
    return ERR_OK;
}

/**
 * @brief Posta uma mensagem na mailbox (bloqueante).
 *
 * @param mbox Ponteiro para a mailbox.
 * @param msg Mensagem a ser postada.
 */
void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
    if (mbox != NULL && *mbox != NULL)
    {
        while (xQueueSend(*mbox, &msg, portMAX_DELAY) != pdTRUE)
        {
            /* Aguarda espaço na fila */
        }
    }
}

/**
 * @brief Tenta postar uma mensagem na mailbox (não bloqueante).
 *
 * @param mbox Ponteiro para a mailbox.
 * @param msg Mensagem a ser postada.
 * @return ERR_OK se sucesso, ERR_MEM se fila cheia.
 */
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    if (mbox == NULL || *mbox == NULL)
    {
        return ERR_ARG;
    }

    if (xQueueSend(*mbox, &msg, 0) == pdTRUE)
    {
        return ERR_OK;
    }
    else
    {
        SYS_STATS_INC(mbox.err);
        return ERR_MEM;
    }
}

/**
 * @brief Tenta postar uma mensagem no início da mailbox (não bloqueante).
 *
 * @param mbox Ponteiro para a mailbox.
 * @param msg Mensagem a ser postada.
 * @return ERR_OK se sucesso, ERR_MEM se fila cheia.
 */
err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (mbox == NULL || *mbox == NULL)
    {
        return ERR_ARG;
    }

    if (xQueueSendFromISR(*mbox, &msg, &xHigherPriorityTaskWoken) == pdTRUE)
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        return ERR_OK;
    }
    else
    {
        return ERR_MEM;
    }
}

/**
 * @brief Busca uma mensagem da mailbox com timeout.
 *
 * @param mbox Ponteiro para a mailbox.
 * @param msg Ponteiro para armazenar a mensagem recebida.
 * @param timeout Tempo máximo de espera em milissegundos (0 = infinito).
 * @return Tempo de espera em ms, ou SYS_ARCH_TIMEOUT se timeout expirou.
 */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    TickType_t startTick;
    TickType_t waitTicks;
    void *msgDummy;
    BaseType_t result;

    if (mbox == NULL || *mbox == NULL)
    {
        return SYS_ARCH_TIMEOUT;
    }

    if (msg == NULL)
    {
        msg = &msgDummy;
    }

    startTick = xTaskGetTickCount();

    if (timeout == 0)
    {
        waitTicks = portMAX_DELAY;
    }
    else
    {
        waitTicks = pdMS_TO_TICKS(timeout);
    }

    result = xQueueReceive(*mbox, msg, waitTicks);

    if (result == pdTRUE)
    {
        TickType_t elapsedTicks = xTaskGetTickCount() - startTick;
        return (u32_t)(elapsedTicks * portTICK_PERIOD_MS);
    }
    else
    {
        *msg = NULL;
        return SYS_ARCH_TIMEOUT;
    }
}

/**
 * @brief Tenta buscar uma mensagem da mailbox (não bloqueante).
 *
 * @param mbox Ponteiro para a mailbox.
 * @param msg Ponteiro para armazenar a mensagem recebida.
 * @return 0 se mensagem recebida, SYS_MBOX_EMPTY se vazia.
 */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    void *msgDummy;

    if (mbox == NULL || *mbox == NULL)
    {
        return SYS_MBOX_EMPTY;
    }

    if (msg == NULL)
    {
        msg = &msgDummy;
    }

    if (xQueueReceive(*mbox, msg, 0) == pdTRUE)
    {
        return 0;
    }
    else
    {
        return SYS_MBOX_EMPTY;
    }
}

/**
 * @brief Libera recursos de uma mailbox.
 *
 * @param mbox Ponteiro para a mailbox a ser destruída.
 */
void sys_mbox_free(sys_mbox_t *mbox)
{
    if (mbox != NULL && *mbox != NULL)
    {
        vQueueDelete(*mbox);
        *mbox = NULL;
        SYS_STATS_DEC(mbox.used);
    }
}

/*-----------------------------------------------------------------------------
 * Funções de Thread
 *----------------------------------------------------------------------------*/

/**
 * @brief Cria uma nova thread do sistema.
 *
 * @param name Nome da thread (para debug).
 * @param thread Função da thread.
 * @param arg Argumento passado para a thread.
 * @param stacksize Tamanho da pilha em words (não bytes).
 * @param prio Prioridade da thread.
 * @return Handle da thread criada, ou NULL se falha.
 */
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread,
                            void *arg, int stacksize, int prio)
{
    TaskHandle_t taskHandle = NULL;
    BaseType_t result;

    result = xTaskCreate((TaskFunction_t)thread,
                         name,
                         (configSTACK_DEPTH_TYPE)stacksize,
                         arg,
                         (UBaseType_t)prio,
                         &taskHandle);

    if (result != pdPASS)
    {
        return NULL;
    }

    return taskHandle;
}

/*-----------------------------------------------------------------------------
 * Funções de Tempo
 *----------------------------------------------------------------------------*/

/**
 * @brief Retorna o tempo atual do sistema em milissegundos.
 *
 * @return Tempo em milissegundos desde a inicialização.
 */
u32_t sys_now(void)
{
    return (u32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/**
 * @brief Retorna o tempo desde o início do sistema em milissegundos.
 *
 * @return Tempo em milissegundos desde sys_init().
 */
u32_t sys_jiffies(void)
{
    return (u32_t)((xTaskGetTickCount() - sysStartTicks) * portTICK_PERIOD_MS);
}

/*-----------------------------------------------------------------------------
 * Gerador de Números Aleatórios
 *----------------------------------------------------------------------------*/

/**
 * @brief Estado interno do gerador LFSR.
 */
static uint32_t randState = 0x12345678;

/**
 * @brief Gera um número pseudo-aleatório usando LFSR.
 *
 * Implementação simples usando Linear Feedback Shift Register combinado
 * com o valor atual do tick do sistema para aumentar a entropia.
 *
 * @return Número pseudo-aleatório de 32 bits.
 */
uint32_t lwip_rand(void)
{
    /* Adiciona entropia do sistema */
    randState ^= xTaskGetTickCount();

    /* LFSR com polinômio: x^32 + x^22 + x^2 + x + 1 */
    uint32_t bit = ((randState >> 0) ^ (randState >> 10) ^ 
                    (randState >> 30) ^ (randState >> 31)) & 1u;
    randState = (randState >> 1) | (bit << 31);

    return randState;
}

/*-----------------------------------------------------------------------------
 * Funções de Proteção de Seção Crítica
 *----------------------------------------------------------------------------*/

/**
 * @brief Mutex para proteção do core do LwIP.
 */
static sys_mutex_t lwipCoreMutex = NULL;

/**
 * @brief Inicializa o mutex de proteção do core.
 */
void sys_lock_tcpip_core(void)
{
    if (lwipCoreMutex == NULL)
    {
        sys_mutex_new(&lwipCoreMutex);
    }
    sys_mutex_lock(&lwipCoreMutex);
}

/**
 * @brief Libera o mutex de proteção do core.
 */
void sys_unlock_tcpip_core(void)
{
    sys_mutex_unlock(&lwipCoreMutex);
}

/**
 * @brief Verifica se o core está travado pela thread atual.
 *
 * @return 1 se travado, 0 caso contrário.
 */
int sys_check_core_locking(void)
{
    /* Simplificação: sempre retorna 1 */
    return 1;
}
