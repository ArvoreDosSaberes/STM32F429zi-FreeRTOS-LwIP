/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Carlos Delfino <consultoria@carlosdelfino.eti.br>
 * @brief          : Programa principal - Servidor HTTP com FreeRTOS e LwIP
 ******************************************************************************
 * @attention
 *
 * Este projeto é um template para desenvolvimento com STM32F429ZI.
 * Integra FreeRTOS para gerenciamento de tarefas e LwIP para conectividade
 * Ethernet com servidor HTTP integrado.
 *
 * GitHub: https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP
 * Site: https://mcu.tec.br
 * YouTube: https://youtube.com/@mcu_fpga
 *
 ******************************************************************************
 */

#include <stdint.h>
#include <stdio.h>

#include "stm32f4xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "portable.h"

#include "httpserver.h"
#include "system_config.h"
#include "mqtt_service.h"

/*-----------------------------------------------------------------------------
 * Declarações Antecipadas (Forward Declarations)
 *----------------------------------------------------------------------------*/

/* Função de inicialização do hardware do sistema */
static void systemHardwareInit(void);

/* Tarefa principal da aplicação */
static void appMainTask(void *pvParameters);

/* Tarefa de monitoramento do sistema */
static void appMonitorTask(void *pvParameters);

/* Tarefa do serviço MQTT */
static void appMqttTask(void *pvParameters);


/*-----------------------------------------------------------------------------
 * Função Principal
 *----------------------------------------------------------------------------*/

/**
 * @brief Função principal do sistema.
 *7
 * Inicializa o hardware, cria as tarefas do FreeRTOS e inicia o escalonador.
 * Esta função nunca retorna em operação normal.
 *
 * @return int Nunca retorna.
 */
int main(void)
{
    /* Inicializar hardware do sistema */
    systemHardwareInit();

    /* Criar tarefa principal da aplicação */
    xTaskCreate(appMainTask,
                "Main",
                512,
                NULL,
                2,
                NULL);

    /* Criar tarefa de monitoramento */
    xTaskCreate(appMonitorTask,
                "Monitor",
                256,
                NULL,
                1,
                NULL);

    /* Criar tarefa MQTT */
    xTaskCreate(appMqttTask,
                "MQTTInit",
                512,
                NULL,
                2,
                NULL);

    /* Iniciar o escalonador do FreeRTOS */
    vTaskStartScheduler();

    /* Loop infinito - execução nunca deve chegar aqui */
    /* Se chegou aqui, houve falha na alocação de memória para o escalonador */
    for (;;)
    {
        /* Tratamento de erro crítico */
    }
}

/*-----------------------------------------------------------------------------
 * Funções de Inicialização do Sistema
 *----------------------------------------------------------------------------*/

/**
 * @brief Inicializa o hardware do sistema.
 *
 * Configura os clocks, GPIOs e periféricos necessários para o funcionamento
 * do microcontrolador STM32F429ZI.
 */
static void systemHardwareInit(void)
{
    /* Inicializar HAL (configura SysTick, Flash, etc.) */
    HAL_Init();

    /* Configurar clocks do sistema */
    systemClockConfig();

    /* Inicializar UART3 para printf (115200 baud) */
    uart3Init();

    /* Agora testar printf */
    printf("\n\r=== Sistema inicializado ===\n\r");
    printf("SYSCLK: %lu Hz\n\r", HAL_RCC_GetSysClockFreq());
    printf("HCLK:   %lu Hz\n\r", HAL_RCC_GetHCLKFreq());
    printf("PCLK1:  %lu Hz\n\r", HAL_RCC_GetPCLK1Freq());
    printf("PCLK2:  %lu Hz\n\r", HAL_RCC_GetPCLK2Freq());
}

/*-----------------------------------------------------------------------------
 * Implementação das Tarefas
 *----------------------------------------------------------------------------*/

/**
 * @brief Tarefa principal da aplicação.
 *
 * Inicializa a pilha de rede LwIP e o servidor HTTP.
 * Após a inicialização, monitora o estado da conexão.
 *
 * @param pvParameters Parâmetros da tarefa (não utilizado).
 */
static void appMainTask(void *pvParameters)
{
    (void)pvParameters;

    char ipBuffer[16];

    /* Aguardar estabilização do sistema */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Inicializar servidor HTTP */
    printf("\n\r=== STM32F429ZI HTTP Server ===\n\r");
    printf("Inicializando stack de rede...\n\r");

    if (httpServerInit() != 0)
    {
        printf("ERRO: Falha ao inicializar servidor HTTP!\n\r");
        
        /* Loop de erro */
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    printf("Servidor HTTP inicializado com sucesso!\n\r");
    printf("Aguardando IP via DHCP...\n\r");

    /* Loop principal - monitorar estado da rede */
    for (;;)
    {
        if (httpServerGetIpAddress(ipBuffer))
        {
            printf("Servidor disponivel em: http://%s/\n\r", ipBuffer);
        }

        /* Verificar estado da conexão a cada 5 segundos */
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief Tarefa de monitoramento do sistema.
 *
 * Monitora o estado do sistema, incluindo uso de memória e
 * estado das tarefas. Útil para debug e diagnóstico.
 *
 * @param pvParameters Parâmetros da tarefa (não utilizado).
 */
static void appMonitorTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        /* Monitorar heap livre */
        size_t freeHeap = xPortGetFreeHeapSize();
        size_t minFreeHeap = xPortGetMinimumEverFreeHeapSize();

        /* Log periódico de status (a cada 30 segundos) */
        printf("[Monitor] Heap livre: %u bytes, Minimo: %u bytes\n\r",
               (unsigned int)freeHeap, (unsigned int)minFreeHeap);

        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

/**
 * @brief Tarefa de inicialização e gerenciamento do serviço MQTT.
 *
 * Aguarda a rede estar disponível (IP atribuído via DHCP) e então
 * inicializa e inicia o serviço MQTT para publicar temperatura
 * e receber comandos do ventilador.
 *
 * @param pvParameters Parâmetros da tarefa (não utilizado).
 */
static void appMqttTask(void *pvParameters)
{
    (void)pvParameters;
    
    char ipBuffer[16];
    MqttServiceError mqttErr;
    
    printf("[MQTTInit] Aguardando IP via DHCP...\r\n");
    
    /* Aguardar até ter um IP válido */
    while (!httpServerGetIpAddress(ipBuffer))
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    printf("[MQTTInit] IP obtido: %s\r\n", ipBuffer);
    printf("[MQTTInit] Iniciando servico MQTT...\r\n");
    
    /* Inicializar serviço MQTT */
    mqttErr = mqttServiceInit();
    if (mqttErr != MQTT_SERVICE_OK)
    {
        printf("[MQTTInit] Erro ao inicializar MQTT: %d\r\n", mqttErr);
        
        /* Manter tarefa rodando mas em estado de erro */
        for (;;)
        {
            printf("[MQTTInit] Servico MQTT em estado de erro\r\n");
            vTaskDelay(pdMS_TO_TICKS(30000));
        }
    }
    
    /* Iniciar tarefa do serviço MQTT */
    mqttErr = mqttServiceStart();
    if (mqttErr != MQTT_SERVICE_OK)
    {
        printf("[MQTTInit] Erro ao iniciar tarefa MQTT: %d\r\n", mqttErr);
        
        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(30000));
        }
    }
    
    printf("[MQTTInit] Servico MQTT iniciado com sucesso!\r\n");
    
    /* Monitorar status do serviço MQTT periodicamente */
    for (;;)
    {
        MqttServiceStatus status;
        mqttServiceGetStatus(&status);
        
        printf("[MQTTInit] MQTT Status - Estado: %d, Msgs Pub: %lu, Msgs Rcv: %lu, Reconexoes: %lu\r\n",
               status.state,
               (unsigned long)status.messagesPublished,
               (unsigned long)status.messagesReceived,
               (unsigned long)status.reconnectCount);
        
        printf("[MQTTInit] Ultima temp: %.1f C, Ventilador: %s\r\n",
               status.lastTemperature,
               status.fanState ? "ON" : "OFF");
        
        vTaskDelay(pdMS_TO_TICKS(60000));  /* Log a cada 60 segundos */
    }
}

/*-----------------------------------------------------------
 * Callbacks obrigatórios do FreeRTOS (conforme FreeRTOSConfig.h)
 *----------------------------------------------------------*/

/**
 * @brief Callback chamado quando há estouro de pilha.
 *
 * @param xTask Handle da tarefa que estourou a pilha.
 * @param pcTaskName Nome da tarefa.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;

    /* Parar execução em caso de estouro de pilha */
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

/**
 * @brief Callback chamado quando alocação de memória falha.
 */
void vApplicationMallocFailedHook(void)
{
    /* Parar execução em caso de falha de alocação */
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

/**
 * @brief Função para prover memória estática para a tarefa Idle.
 *
 * @param ppxIdleTaskTCBBuffer Ponteiro para o TCB da tarefa Idle.
 * @param ppxIdleTaskStackBuffer Ponteiro para a pilha da tarefa Idle.
 * @param pulIdleTaskStackSize Tamanho da pilha.
 */
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

/**
 * @brief Função para prover memória estática para a tarefa Timer.
 *
 * @param ppxTimerTaskTCBBuffer Ponteiro para o TCB da tarefa Timer.
 * @param ppxTimerTaskStackBuffer Ponteiro para a pilha da tarefa Timer.
 * @param pulTimerTaskStackSize Tamanho da pilha.
 */
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
