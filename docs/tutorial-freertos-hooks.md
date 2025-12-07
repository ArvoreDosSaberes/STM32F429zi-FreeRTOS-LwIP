# Tutorial: Funções de Callback (Hooks) do FreeRTOS

Este tutorial explica como ativar e utilizar as principais funções de callback do FreeRTOS para tratamento de erros e alocação estática de memória.

---

## Índice

1. [Visão Geral](#visão-geral)
2. [Configuração no FreeRTOSConfig.h](#configuração-no-freertoscconfigh)
3. [vApplicationStackOverflowHook](#vapplicationstackoverflowhook)
4. [vApplicationMallocFailedHook](#vapplicationmallocfailedhook)
5. [vApplicationGetIdleTaskMemory](#vapplicationgetidletaskmemory)
6. [vApplicationGetTimerTaskMemory](#vapplicationgettimertaskmemory)
7. [Estratégias de Uso e Boas Práticas](#estratégias-de-uso-e-boas-práticas)
8. [Exemplo Completo](#exemplo-completo)

---

## Visão Geral

O FreeRTOS utiliza um sistema de **callbacks** (também chamados de *hooks*) para permitir que a aplicação personalize o comportamento do kernel em situações específicas. Essas funções são chamadas automaticamente pelo FreeRTOS quando determinados eventos ocorrem.

### Tipos de Callbacks

| Callback | Propósito |
|----------|-----------|
| `vApplicationStackOverflowHook` | Detecção de estouro de pilha |
| `vApplicationMallocFailedHook` | Tratamento de falha de alocação de memória |
| `vApplicationGetIdleTaskMemory` | Fornecimento de memória estática para a tarefa Idle |
| `vApplicationGetTimerTaskMemory` | Fornecimento de memória estática para a tarefa Timer |

---

## Configuração no FreeRTOSConfig.h

Para ativar cada callback, é necessário definir macros específicas no arquivo `FreeRTOSConfig.h`:

```c
/* Habilita detecção de estouro de pilha (1 ou 2) */
#define configCHECK_FOR_STACK_OVERFLOW          2

/* Habilita callback de falha de malloc */
#define configUSE_MALLOC_FAILED_HOOK            1

/* Habilita alocação estática (requer as funções Get*Memory) */
#define configSUPPORT_STATIC_ALLOCATION         1

/* Habilita software timers (requer vApplicationGetTimerTaskMemory) */
#define configUSE_TIMERS                        1
```

### Tabela de Dependências

| Macro | Valor | Callback Requerido |
|-------|-------|-------------------|
| `configCHECK_FOR_STACK_OVERFLOW` | 1 ou 2 | `vApplicationStackOverflowHook` |
| `configUSE_MALLOC_FAILED_HOOK` | 1 | `vApplicationMallocFailedHook` |
| `configSUPPORT_STATIC_ALLOCATION` | 1 | `vApplicationGetIdleTaskMemory` |
| `configUSE_TIMERS` + `configSUPPORT_STATIC_ALLOCATION` | 1 | `vApplicationGetTimerTaskMemory` |

---

## vApplicationStackOverflowHook

### O que é?

Callback chamado automaticamente pelo FreeRTOS quando detecta que uma tarefa ultrapassou os limites de sua pilha. Este é um dos erros mais comuns em sistemas embarcados e pode causar comportamento imprevisível.

### Assinatura

```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName);
```

### Parâmetros

| Parâmetro | Tipo | Descrição |
|-----------|------|-----------|
| `xTask` | `TaskHandle_t` | Handle da tarefa que causou o estouro |
| `pcTaskName` | `char*` | Nome da tarefa (string) |

### Ativação

No `FreeRTOSConfig.h`, defina:

```c
#define configCHECK_FOR_STACK_OVERFLOW    2
```

### Níveis de Verificação

- **Nível 1**: Verifica apenas se o ponteiro de pilha saiu dos limites. Mais rápido, mas menos confiável.
- **Nível 2**: Além da verificação do nível 1, preenche a pilha com um padrão conhecido (0xA5) e verifica se os últimos 16 bytes foram sobrescritos. Mais lento, porém mais confiável.

> **Recomendação**: Use nível 2 durante desenvolvimento e depuração.

### Implementação Recomendada

```c
/**
 * @brief Callback de estouro de pilha.
 *
 * Chamado quando uma tarefa excede o limite de sua pilha.
 * Em produção, pode-se registrar o erro e tentar reiniciar o sistema.
 *
 * @param xTask Handle da tarefa problemática.
 * @param pcTaskName Nome da tarefa.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
    (void)xTask;  /* Evita warning de parâmetro não utilizado */

    /* Opção 1: Log do erro (debug) */
    printf("ERRO: Stack overflow na tarefa '%s'\r\n", pcTaskName);

    /* Opção 2: Desabilitar interrupções e parar (seguro) */
    taskDISABLE_INTERRUPTS();
    
    /* Opção 3: Acender LED de erro */
    /* HAL_GPIO_WritePin(LED_ERROR_GPIO_Port, LED_ERROR_Pin, GPIO_PIN_SET); */

    /* Loop infinito - sistema travado */
    for (;;)
    {
        /* Em debug, pode-se adicionar um breakpoint aqui */
    }
}
```

### Estratégias de Uso

1. **Durante desenvolvimento**: Utilize `printf` ou UART para identificar a tarefa problemática.
2. **Em produção**: Grave o nome da tarefa em memória não-volátil (Flash/EEPROM) e reinicie o sistema.
3. **Monitoramento**: Use `uxTaskGetStackHighWaterMark()` periodicamente para detectar tarefas com pilha quase esgotada.

---

## vApplicationMallocFailedHook

### O que é?

Callback invocado quando uma alocação dinâmica de memória (`pvPortMalloc`) falha por falta de heap disponível. Indica que o sistema está com memória esgotada.

### Assinatura

```c
void vApplicationMallocFailedHook(void);
```

### Ativação

No `FreeRTOSConfig.h`, defina:

```c
#define configUSE_MALLOC_FAILED_HOOK    1
```

### Implementação Recomendada

```c
/**
 * @brief Callback de falha de alocação de memória.
 *
 * Chamado quando pvPortMalloc() retorna NULL por falta de heap.
 * Indica problema grave de dimensionamento de memória.
 */
void vApplicationMallocFailedHook(void)
{
    /* Log do erro (se UART disponível) */
    printf("ERRO: Falha de alocação de memória (heap esgotado)\r\n");

    /* Informações úteis para debug */
    size_t freeHeap = xPortGetFreeHeapSize();
    size_t minEverFree = xPortGetMinimumEverFreeHeapSize();
    printf("  Heap livre: %u bytes\r\n", (unsigned int)freeHeap);
    printf("  Mínimo histórico: %u bytes\r\n", (unsigned int)minEverFree);

    /* Desabilitar interrupções e parar */
    taskDISABLE_INTERRUPTS();

    for (;;)
    {
        /* Sistema travado */
    }
}
```

### Estratégias de Uso

1. **Dimensionar corretamente o heap**: Ajuste `configTOTAL_HEAP_SIZE` baseado no uso real.
2. **Monitorar heap**: Use `xPortGetFreeHeapSize()` e `xPortGetMinimumEverFreeHeapSize()` regularmente.
3. **Preferir alocação estática**: Para sistemas críticos, use `configSUPPORT_STATIC_ALLOCATION` e elimine alocações dinâmicas.
4. **Evitar fragmentação**: Prefira `heap_4.c` ou `heap_5.c` que coalescem blocos livres.

---

## vApplicationGetIdleTaskMemory

### O que é?

Função obrigatória quando `configSUPPORT_STATIC_ALLOCATION` está habilitado. O FreeRTOS chama esta função durante `vTaskStartScheduler()` para obter a memória necessária para criar a tarefa Idle.

### Assinatura

```c
void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer,
                                   StackType_t** ppxIdleTaskStackBuffer,
                                   uint32_t* pulIdleTaskStackSize);
```

### Parâmetros

| Parâmetro | Tipo | Descrição |
|-----------|------|-----------|
| `ppxIdleTaskTCBBuffer` | `StaticTask_t**` | Ponteiro para receber o endereço do TCB (Task Control Block) |
| `ppxIdleTaskStackBuffer` | `StackType_t**` | Ponteiro para receber o endereço da pilha |
| `pulIdleTaskStackSize` | `uint32_t*` | Ponteiro para receber o tamanho da pilha (em palavras) |

### Ativação

No `FreeRTOSConfig.h`, defina:

```c
#define configSUPPORT_STATIC_ALLOCATION    1
```

### Implementação Recomendada

```c
/**
 * @brief Fornece memória estática para a tarefa Idle.
 *
 * Chamado automaticamente pelo FreeRTOS durante vTaskStartScheduler().
 * A memória deve ser estática e persistir durante toda a execução.
 *
 * @param ppxIdleTaskTCBBuffer Recebe ponteiro para o TCB.
 * @param ppxIdleTaskStackBuffer Recebe ponteiro para a pilha.
 * @param pulIdleTaskStackSize Recebe tamanho da pilha (em StackType_t).
 */
void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer,
                                   StackType_t** ppxIdleTaskStackBuffer,
                                   uint32_t* pulIdleTaskStackSize)
{
    /* Buffers estáticos - alocados em tempo de compilação */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    /* Retorna os ponteiros para os buffers */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
```

### Pontos Importantes

1. **Variáveis estáticas**: Os buffers DEVEM ser `static` para persistir após o retorno da função.
2. **Tamanho da pilha**: Use `configMINIMAL_STACK_SIZE` definido em `FreeRTOSConfig.h`.
3. **TCB (Task Control Block)**: Estrutura interna do FreeRTOS que armazena o estado da tarefa.

---

## vApplicationGetTimerTaskMemory

### O que é?

Similar à função anterior, mas fornece memória para a tarefa de serviço de timers (Timer Service Task ou *Daemon Task*). Esta tarefa processa os comandos de software timers.

### Assinatura

```c
void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer,
                                    StackType_t** ppxTimerTaskStackBuffer,
                                    uint32_t* pulTimerTaskStackSize);
```

### Ativação

No `FreeRTOSConfig.h`, defina:

```c
#define configSUPPORT_STATIC_ALLOCATION    1
#define configUSE_TIMERS                   1
```

### Implementação Recomendada

```c
/**
 * @brief Fornece memória estática para a tarefa Timer (daemon).
 *
 * Chamado automaticamente quando software timers são utilizados.
 * A pilha deve ser maior que a da tarefa Idle se callbacks de
 * timer executarem operações complexas.
 *
 * @param ppxTimerTaskTCBBuffer Recebe ponteiro para o TCB.
 * @param ppxTimerTaskStackBuffer Recebe ponteiro para a pilha.
 * @param pulTimerTaskStackSize Recebe tamanho da pilha (em StackType_t).
 */
void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer,
                                    StackType_t** ppxTimerTaskStackBuffer,
                                    uint32_t* pulTimerTaskStackSize)
{
    /* Buffers estáticos */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    /* Retorna os ponteiros */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
```

### Configurações Relacionadas

```c
/* Prioridade da tarefa Timer - geralmente alta */
#define configTIMER_TASK_PRIORITY       ( configMAX_PRIORITIES - 1 )

/* Tamanho da fila de comandos de timer */
#define configTIMER_QUEUE_LENGTH        10

/* Tamanho da pilha (deve acomodar callbacks de timer) */
#define configTIMER_TASK_STACK_DEPTH    ( configMINIMAL_STACK_SIZE * 2 )
```

---

## Estratégias de Uso e Boas Práticas

### 1. Alocação Estática vs Dinâmica

| Aspecto | Estática | Dinâmica |
|---------|----------|----------|
| **Previsibilidade** | Alta | Baixa (fragmentação) |
| **Uso de RAM** | Fixo | Variável |
| **Falhas em runtime** | Impossível | Possível |
| **Flexibilidade** | Baixa | Alta |

> **Recomendação**: Para sistemas críticos (automotivo, médico, industrial), prefira alocação estática.

### 2. Dimensionamento de Pilha

```c
/* Verificar uso de pilha em runtime */
void vMonitorTask(void* pvParameters)
{
    for (;;)
    {
        UBaseType_t highWaterMark;
        
        /* Obtém menor quantidade de pilha livre (em palavras) */
        highWaterMark = uxTaskGetStackHighWaterMark(NULL);
        
        printf("Pilha livre mínima: %u palavras\r\n", highWaterMark);
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
```

### 3. Tratamento de Erros em Produção

```c
/* Estrutura para log de erros em Flash */
typedef struct {
    uint32_t timestamp;
    uint8_t  errorType;      /* 1=StackOverflow, 2=MallocFailed */
    char     taskName[16];
    uint32_t freeHeap;
} ErrorLog_t;

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
    ErrorLog_t log = {0};
    
    log.timestamp = xTaskGetTickCount();
    log.errorType = 1;
    strncpy(log.taskName, pcTaskName, sizeof(log.taskName) - 1);
    
    /* Salvar em Flash antes de reiniciar */
    Flash_WriteErrorLog(&log);
    
    /* Reiniciar sistema */
    NVIC_SystemReset();
}
```

### 4. Checklist de Configuração

- [ ] `configCHECK_FOR_STACK_OVERFLOW` = 2 (desenvolvimento)
- [ ] `configUSE_MALLOC_FAILED_HOOK` = 1
- [ ] `configSUPPORT_STATIC_ALLOCATION` = 1 (sistemas críticos)
- [ ] Implementar todos os callbacks obrigatórios
- [ ] Monitorar `uxTaskGetStackHighWaterMark()` em desenvolvimento
- [ ] Monitorar `xPortGetMinimumEverFreeHeapSize()` em desenvolvimento

---

## Exemplo Completo

Arquivo `freertos_hooks.c`:

```c
/**
 * @file freertos_hooks.c
 * @brief Implementação dos callbacks obrigatórios do FreeRTOS.
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/*-----------------------------------------------------------
 * Hook de estouro de pilha
 *----------------------------------------------------------*/
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
    (void)xTask;
    
    printf("FATAL: Stack overflow em '%s'\r\n", pcTaskName);
    
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

/*-----------------------------------------------------------
 * Hook de falha de malloc
 *----------------------------------------------------------*/
void vApplicationMallocFailedHook(void)
{
    printf("FATAL: Malloc failed. Heap livre: %u\r\n", 
           (unsigned int)xPortGetFreeHeapSize());
    
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

/*-----------------------------------------------------------
 * Memória estática para tarefa Idle
 *----------------------------------------------------------*/
void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer,
                                   StackType_t** ppxIdleTaskStackBuffer,
                                   uint32_t* pulIdleTaskStackSize)
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/*-----------------------------------------------------------
 * Memória estática para tarefa Timer
 *----------------------------------------------------------*/
void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer,
                                    StackType_t** ppxTimerTaskStackBuffer,
                                    uint32_t* pulTimerTaskStackSize)
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
```

---

## Referências

- [FreeRTOS - Stack Overflow Detection](https://www.freertos.org/Stacks-and-stack-overflow-checking.html)
- [FreeRTOS - Static vs Dynamic Memory Allocation](https://www.freertos.org/Static_Vs_Dynamic_Memory_Allocation.html)
- [FreeRTOS - Memory Management](https://www.freertos.org/a00111.html)
- [FreeRTOS - Software Timers](https://www.freertos.org/FreeRTOS-Software-Timer-API-Functions.html)

---

*Documento criado para o projeto STM32F429ZI-FreeRTOS-LwIP*
