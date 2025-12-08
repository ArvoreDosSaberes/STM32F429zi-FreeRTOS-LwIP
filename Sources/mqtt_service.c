/**
 * @file mqtt_service.c
 * @brief Implementação do serviço MQTT para STM32F429ZI com FreeRTOS.
 * 
 * Gerencia conexão com broker MQTT, publicação de temperatura e
 * subscrição para controle de ventilador usando LwIP MQTT client
 * e FreeRTOS para concorrência.
 */

#include "mqtt_service.h"
#include "temperature_sensor.h"
#include "fan_controller.h"

#include "stm32f4xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "lwip/apps/mqtt.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*-----------------------------------------------------------------------------
 * Definições Privadas
 *----------------------------------------------------------------------------*/

/** @brief Tamanho do stack da tarefa MQTT em palavras. */
#define MQTT_TASK_STACK_SIZE        256

/** @brief Prioridade da tarefa MQTT. */
#define MQTT_TASK_PRIORITY          (tskIDLE_PRIORITY + 2)

/** @brief Tamanho máximo do buffer para payloads recebidos. */
#define MQTT_RX_BUFFER_SIZE         128

/** @brief Tamanho máximo do buffer para publicação. */
#define MQTT_TX_BUFFER_SIZE         64

/*-----------------------------------------------------------------------------
 * Variáveis Privadas
 *----------------------------------------------------------------------------*/

/** @brief Cliente MQTT do LwIP. */
static mqtt_client_t *mqttClient = NULL;

/** @brief Estado atual do serviço. */
static volatile MqttServiceState serviceState = MQTT_SERVICE_DISCONNECTED;

/** @brief Handle da tarefa MQTT. */
static TaskHandle_t mqttTaskHandle = NULL;

/** @brief Mutex para acesso thread-safe às variáveis. */
static SemaphoreHandle_t mqttMutex = NULL;

/** @brief Callback externo para mensagens recebidas. */
static MqttMessageCallback userCallback = NULL;

/** @brief Buffer para receber payloads de mensagens. */
static uint8_t rxBuffer[MQTT_RX_BUFFER_SIZE];

/** @brief Posição atual no buffer de recepção. */
static uint16_t rxBufferPos = 0;

/** @brief Tópico da mensagem sendo recebida. */
static char rxTopic[64];

/** @brief Tamanho do tópico da mensagem sendo recebida. */
static uint16_t rxTopicLen = 0;

/** @brief Estatísticas do serviço. */
static MqttServiceStatus serviceStatus = {0};

/** @brief Endereço IP do broker (resolvido via DNS). */
static ip_addr_t brokerIpAddr;

/** @brief Flag indicando se DNS foi resolvido. */
static volatile bool dnsResolved = false;

/** @brief Flag para sinalizar reconexão. */
static volatile bool needReconnect = false;

/*-----------------------------------------------------------------------------
 * Protótipos de Funções Privadas
 *----------------------------------------------------------------------------*/

static void mqttTask(void *pvParameters);
static void mqttConnectionCallback(mqtt_client_t *client, void *arg, 
                                    mqtt_connection_status_t status);
static void mqttIncomingPublishCallback(void *arg, const char *topic, 
                                         u32_t tot_len);
static void mqttIncomingDataCallback(void *arg, const u8_t *data, 
                                      u16_t len, u8_t flags);
static void mqttSubscribeCallback(void *arg, err_t result);
static void mqttPublishCallback(void *arg, err_t result);
static void dnsFoundCallback(const char *name, const ip_addr_t *ipaddr, void *arg);
static err_t connectToBroker(void);
static void processReceivedMessage(const char *topic, uint16_t topicLen,
                                   const uint8_t *payload, uint16_t payloadLen);
static void subscribeToTopics(void);

/*-----------------------------------------------------------------------------
 * Implementação das Funções Públicas
 *----------------------------------------------------------------------------*/

MqttServiceError mqttServiceInit(void)
{
    printf("[MQTT] Inicializando servico MQTT...\r\n");
    
    /* Criar mutex para thread-safety */
    mqttMutex = xSemaphoreCreateMutex();
    if (mqttMutex == NULL)
    {
        printf("[MQTT] Erro: Falha ao criar mutex\r\n");
        return MQTT_SERVICE_ERR_INIT;
    }
    
    /* Inicializar sensor de temperatura */
    if (temperatureSensorInit() != TEMP_SENSOR_OK)
    {
        printf("[MQTT] Erro: Falha ao inicializar sensor de temperatura\r\n");
        return MQTT_SERVICE_ERR_INIT;
    }
    
    /* Inicializar controlador do ventilador */
    if (fanControllerInit() != 0)
    {
        printf("[MQTT] Erro: Falha ao inicializar controlador do ventilador\r\n");
        return MQTT_SERVICE_ERR_INIT;
    }
    
    /* Criar cliente MQTT */
    mqttClient = mqtt_client_new();
    if (mqttClient == NULL)
    {
        printf("[MQTT] Erro: Falha ao criar cliente MQTT\r\n");
        return MQTT_SERVICE_ERR_INIT;
    }
    
    /* Inicializar estatísticas */
    memset(&serviceStatus, 0, sizeof(serviceStatus));
    serviceStatus.state = MQTT_SERVICE_DISCONNECTED;
    
    printf("[MQTT] Servico inicializado com sucesso\r\n");
    printf("[MQTT] Broker: %s:%d\r\n", MQTT_BROKER, MQTT_BROKER_PORT);
    printf("[MQTT] Client ID: %s\r\n", MQTT_CLIENT_ID);
    printf("[MQTT] Topico temp: %s\r\n", MQTT_TOPIC_TEMPERATURE);
    printf("[MQTT] Topico fan cmd: %s\r\n", MQTT_TOPIC_FAN_COMMAND);
    
    return MQTT_SERVICE_OK;
}

MqttServiceError mqttServiceStart(void)
{
    BaseType_t result;
    
    if (mqttClient == NULL)
    {
        printf("[MQTT] Erro: Servico nao inicializado\r\n");
        return MQTT_SERVICE_ERR_INIT;
    }
    
    if (mqttTaskHandle != NULL)
    {
        printf("[MQTT] Aviso: Tarefa ja esta em execucao\r\n");
        return MQTT_SERVICE_OK;
    }
    
    /* Criar tarefa MQTT */
    result = xTaskCreate(mqttTask,
                         "MQTT",
                         MQTT_TASK_STACK_SIZE,
                         NULL,
                         MQTT_TASK_PRIORITY,
                         &mqttTaskHandle);
    
    if (result != pdPASS)
    {
        printf("[MQTT] Erro: Falha ao criar tarefa MQTT\r\n");
        return MQTT_SERVICE_ERR_INIT;
    }
    
    printf("[MQTT] Tarefa MQTT iniciada\r\n");
    return MQTT_SERVICE_OK;
}

void mqttServiceStop(void)
{
    printf("[MQTT] Parando servico MQTT...\r\n");
    
    /* Desconectar do broker */
    if (mqttClient != NULL && mqtt_client_is_connected(mqttClient))
    {
        mqtt_disconnect(mqttClient);
    }
    
    /* Deletar tarefa */
    if (mqttTaskHandle != NULL)
    {
        vTaskDelete(mqttTaskHandle);
        mqttTaskHandle = NULL;
    }
    
    serviceState = MQTT_SERVICE_DISCONNECTED;
    printf("[MQTT] Servico parado\r\n");
}

bool mqttServiceIsConnected(void)
{
    return (serviceState == MQTT_SERVICE_READY);
}

MqttServiceState mqttServiceGetState(void)
{
    return serviceState;
}

void mqttServiceGetStatus(MqttServiceStatus *status)
{
    if (status == NULL)
    {
        return;
    }
    
    if (xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        memcpy(status, &serviceStatus, sizeof(MqttServiceStatus));
        status->state = serviceState;
        status->fanState = fanControllerIsOn();
        xSemaphoreGive(mqttMutex);
    }
}

MqttServiceError mqttServicePublishTemperature(void)
{
    float temperature;
    char payload[MQTT_TX_BUFFER_SIZE];
    int payloadLen;
    err_t err;
    
    if (!mqttServiceIsConnected())
    {
        return MQTT_SERVICE_ERR_CONNECT;
    }
    
    /* Ler temperatura */
    if (temperatureSensorRead(&temperature) != TEMP_SENSOR_OK)
    {
        printf("[MQTT] Erro: Falha ao ler temperatura\r\n");
        return MQTT_SERVICE_ERR_PUBLISH;
    }
    
    /* Formatar payload JSON */
    payloadLen = snprintf(payload, sizeof(payload),
                          "{\"temperature\":%.1f,\"unit\":\"C\"}",
                          temperature);
    
    if (payloadLen < 0 || payloadLen >= (int)sizeof(payload))
    {
        return MQTT_SERVICE_ERR_PUBLISH;
    }
    
    /* Publicar */
    err = mqtt_publish(mqttClient,
                       MQTT_TOPIC_TEMPERATURE,
                       payload,
                       (uint16_t)payloadLen,
                       MQTT_QOS,
                       MQTT_RETAIN,
                       mqttPublishCallback,
                       NULL);
    
    if (err != ERR_OK)
    {
        printf("[MQTT] Erro ao publicar temperatura: %d\r\n", err);
        return MQTT_SERVICE_ERR_PUBLISH;
    }
    
    /* Atualizar estatísticas */
    if (xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        serviceStatus.messagesPublished++;
        serviceStatus.lastTemperature = temperature;
        xSemaphoreGive(mqttMutex);
    }
    
    printf("[MQTT] Temperatura publicada: %.1f C\r\n", temperature);
    return MQTT_SERVICE_OK;
}

MqttServiceError mqttServicePublishFanStatus(void)
{
    char payload[MQTT_TX_BUFFER_SIZE];
    int payloadLen;
    err_t err;
    
    if (!mqttServiceIsConnected())
    {
        return MQTT_SERVICE_ERR_CONNECT;
    }
    
    /* Formatar payload JSON */
    payloadLen = snprintf(payload, sizeof(payload),
                          "{\"fan\":\"%s\"}",
                          fanControllerGetStateString());
    
    if (payloadLen < 0 || payloadLen >= (int)sizeof(payload))
    {
        return MQTT_SERVICE_ERR_PUBLISH;
    }
    
    /* Publicar */
    err = mqtt_publish(mqttClient,
                       MQTT_TOPIC_FAN_STATUS,
                       payload,
                       (uint16_t)payloadLen,
                       MQTT_QOS,
                       MQTT_RETAIN,
                       mqttPublishCallback,
                       NULL);
    
    if (err != ERR_OK)
    {
        printf("[MQTT] Erro ao publicar status do ventilador: %d\r\n", err);
        return MQTT_SERVICE_ERR_PUBLISH;
    }
    
    if (xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        serviceStatus.messagesPublished++;
        xSemaphoreGive(mqttMutex);
    }
    
    printf("[MQTT] Status do ventilador publicado: %s\r\n", 
           fanControllerGetStateString());
    return MQTT_SERVICE_OK;
}

MqttServiceError mqttServicePublish(const char *topic, const void *payload,
                                     uint16_t payloadLen, uint8_t qos, bool retain)
{
    err_t err;
    
    if (!mqttServiceIsConnected())
    {
        return MQTT_SERVICE_ERR_CONNECT;
    }
    
    if (topic == NULL || payload == NULL)
    {
        return MQTT_SERVICE_ERR_PUBLISH;
    }
    
    err = mqtt_publish(mqttClient,
                       topic,
                       payload,
                       payloadLen,
                       qos,
                       retain ? 1 : 0,
                       mqttPublishCallback,
                       NULL);
    
    if (err != ERR_OK)
    {
        return MQTT_SERVICE_ERR_PUBLISH;
    }
    
    if (xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        serviceStatus.messagesPublished++;
        xSemaphoreGive(mqttMutex);
    }
    
    return MQTT_SERVICE_OK;
}

void mqttServiceSetMessageCallback(MqttMessageCallback callback)
{
    userCallback = callback;
}

/*-----------------------------------------------------------------------------
 * Implementação das Funções Privadas
 *----------------------------------------------------------------------------*/

/**
 * @brief Tarefa principal do serviço MQTT.
 * 
 * Gerencia o ciclo de vida da conexão MQTT:
 * 1. Resolve DNS do broker
 * 2. Conecta ao broker
 * 3. Subscreve nos tópicos
 * 4. Publica temperatura periodicamente
 * 5. Reconecta se necessário
 * 
 * @param pvParameters Parâmetros da tarefa (não utilizado).
 */
static void mqttTask(void *pvParameters)
{
    (void)pvParameters;
    
    TickType_t lastPublishTime = 0;
    TickType_t currentTime;
    err_t err;
    
    printf("[MQTT] Tarefa iniciada. Aguardando rede...\r\n");
    
    /* Aguardar estabilização da rede */
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    for (;;)
    {
        currentTime = xTaskGetTickCount();
        
        switch (serviceState)
        {
            case MQTT_SERVICE_DISCONNECTED:
                /* Iniciar resolução DNS */
                printf("[MQTT] Resolvendo DNS: %s\r\n", MQTT_BROKER);
                serviceState = MQTT_SERVICE_CONNECTING;
                dnsResolved = false;
                
                err = dns_gethostbyname(MQTT_BROKER, &brokerIpAddr, 
                                        dnsFoundCallback, NULL);
                
                if (err == ERR_OK)
                {
                    /* IP já em cache */
                    dnsResolved = true;
                    printf("[MQTT] DNS em cache: %s\r\n", 
                           ipaddr_ntoa(&brokerIpAddr));
                }
                else if (err != ERR_INPROGRESS)
                {
                    printf("[MQTT] Erro DNS: %d\r\n", err);
                    serviceState = MQTT_SERVICE_ERROR;
                }
                break;
                
            case MQTT_SERVICE_CONNECTING:
                if (dnsResolved)
                {
                    /* DNS resolvido, conectar ao broker */
                    err = connectToBroker();
                    if (err != ERR_OK)
                    {
                        printf("[MQTT] Falha na conexao: %d\r\n", err);
                        serviceState = MQTT_SERVICE_DISCONNECTED;
                        vTaskDelay(pdMS_TO_TICKS(MQTT_RECONNECT_INTERVAL_MS));
                    }
                    /* Estado será atualizado pelo callback de conexão */
                }
                else
                {
                    /* Aguardando DNS */
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                break;
                
            case MQTT_SERVICE_CONNECTED:
                /* Subscrever nos tópicos */
                serviceState = MQTT_SERVICE_SUBSCRIBING;
                subscribeToTopics();
                break;
                
            case MQTT_SERVICE_SUBSCRIBING:
                /* Aguardando subscrição completar */
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
                
            case MQTT_SERVICE_READY:
                /* Operação normal - publicar temperatura periodicamente */
                if ((currentTime - lastPublishTime) >= 
                    pdMS_TO_TICKS(MQTT_PUBLISH_INTERVAL_MS))
                {
                    mqttServicePublishTemperature();
                    lastPublishTime = currentTime;
                }
                
                /* Verificar se precisa reconectar */
                if (needReconnect || !mqtt_client_is_connected(mqttClient))
                {
                    printf("[MQTT] Conexao perdida. Reconectando...\r\n");
                    serviceState = MQTT_SERVICE_DISCONNECTED;
                    needReconnect = false;
                    
                    if (xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        serviceStatus.reconnectCount++;
                        xSemaphoreGive(mqttMutex);
                    }
                    
                    vTaskDelay(pdMS_TO_TICKS(MQTT_RECONNECT_INTERVAL_MS));
                }
                break;
                
            case MQTT_SERVICE_ERROR:
                /* Erro - tentar reconectar após delay */
                printf("[MQTT] Erro. Tentando novamente em %d ms...\r\n",
                       MQTT_RECONNECT_INTERVAL_MS);
                vTaskDelay(pdMS_TO_TICKS(MQTT_RECONNECT_INTERVAL_MS));
                serviceState = MQTT_SERVICE_DISCONNECTED;
                break;
                
            default:
                break;
        }
        
        /* Yield para outras tarefas */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Callback de resolução DNS.
 */
static void dnsFoundCallback(const char *name, const ip_addr_t *ipaddr, void *arg)
{
    (void)name;
    (void)arg;
    
    if (ipaddr != NULL)
    {
        ip_addr_copy(brokerIpAddr, *ipaddr);
        printf("[MQTT] DNS resolvido: %s\r\n", ipaddr_ntoa(&brokerIpAddr));
        dnsResolved = true;
    }
    else
    {
        printf("[MQTT] Falha na resolucao DNS\r\n");
        serviceState = MQTT_SERVICE_ERROR;
    }
}

/**
 * @brief Conecta ao broker MQTT.
 */
static err_t connectToBroker(void)
{
    struct mqtt_connect_client_info_t clientInfo;
    err_t err;
    
    memset(&clientInfo, 0, sizeof(clientInfo));
    
    clientInfo.client_id = MQTT_CLIENT_ID;
    clientInfo.client_user = MQTT_USERNAME;
    clientInfo.client_pass = MQTT_PASSWORD;
    clientInfo.keep_alive = MQTT_KEEP_ALIVE_S;
    clientInfo.will_topic = MQTT_TOPIC_STATUS;
    clientInfo.will_msg = "{\"status\":\"offline\"}";
    clientInfo.will_qos = 1;
    clientInfo.will_retain = 1;
    
    printf("[MQTT] Conectando a %s:%d...\r\n", 
           ipaddr_ntoa(&brokerIpAddr), MQTT_BROKER_PORT);
    
    /* Configurar callbacks de mensagens recebidas */
    mqtt_set_inpub_callback(mqttClient,
                            mqttIncomingPublishCallback,
                            mqttIncomingDataCallback,
                            NULL);
    
    /* Conectar */
    err = mqtt_client_connect(mqttClient,
                              &brokerIpAddr,
                              MQTT_BROKER_PORT,
                              mqttConnectionCallback,
                              NULL,
                              &clientInfo);
    
    return err;
}

/**
 * @brief Callback de status da conexão MQTT.
 */
static void mqttConnectionCallback(mqtt_client_t *client, void *arg,
                                    mqtt_connection_status_t status)
{
    (void)client;
    (void)arg;
    
    switch (status)
    {
        case MQTT_CONNECT_ACCEPTED:
            printf("[MQTT] Conexao aceita!\r\n");
            serviceState = MQTT_SERVICE_CONNECTED;
            
            /* Publicar status online */
            mqtt_publish(mqttClient,
                        MQTT_TOPIC_STATUS,
                        "{\"status\":\"online\"}",
                        20,
                        1, 1,
                        mqttPublishCallback, NULL);
            break;
            
        case MQTT_CONNECT_REFUSED_PROTOCOL_VERSION:
            printf("[MQTT] Conexao recusada: versao do protocolo\r\n");
            serviceState = MQTT_SERVICE_ERROR;
            break;
            
        case MQTT_CONNECT_REFUSED_IDENTIFIER:
            printf("[MQTT] Conexao recusada: identificador invalido\r\n");
            serviceState = MQTT_SERVICE_ERROR;
            break;
            
        case MQTT_CONNECT_REFUSED_SERVER:
            printf("[MQTT] Conexao recusada: servidor indisponivel\r\n");
            serviceState = MQTT_SERVICE_ERROR;
            break;
            
        case MQTT_CONNECT_REFUSED_USERNAME_PASS:
            printf("[MQTT] Conexao recusada: usuario/senha invalidos\r\n");
            serviceState = MQTT_SERVICE_ERROR;
            break;
            
        case MQTT_CONNECT_REFUSED_NOT_AUTHORIZED_:
            printf("[MQTT] Conexao recusada: nao autorizado\r\n");
            serviceState = MQTT_SERVICE_ERROR;
            break;
            
        case MQTT_CONNECT_DISCONNECTED:
            printf("[MQTT] Desconectado do broker\r\n");
            needReconnect = true;
            break;
            
        case MQTT_CONNECT_TIMEOUT:
            printf("[MQTT] Timeout na conexao\r\n");
            serviceState = MQTT_SERVICE_ERROR;
            break;
            
        default:
            printf("[MQTT] Status desconhecido: %d\r\n", status);
            serviceState = MQTT_SERVICE_ERROR;
            break;
    }
}

/**
 * @brief Subscreve nos tópicos configurados.
 */
static void subscribeToTopics(void)
{
    err_t err;
    
    printf("[MQTT] Subscrevendo no topico: %s\r\n", MQTT_TOPIC_FAN_COMMAND);
    
    err = mqtt_subscribe(mqttClient,
                         MQTT_TOPIC_FAN_COMMAND,
                         MQTT_QOS,
                         mqttSubscribeCallback,
                         NULL);
    
    if (err != ERR_OK)
    {
        printf("[MQTT] Erro na subscricao: %d\r\n", err);
        serviceState = MQTT_SERVICE_ERROR;
    }
}

/**
 * @brief Callback de resultado da subscrição.
 */
static void mqttSubscribeCallback(void *arg, err_t result)
{
    (void)arg;
    
    if (result == ERR_OK)
    {
        printf("[MQTT] Subscricao confirmada\r\n");
        serviceState = MQTT_SERVICE_READY;
        
        /* Publicar status inicial do ventilador */
        mqttServicePublishFanStatus();
    }
    else
    {
        printf("[MQTT] Falha na subscricao: %d\r\n", result);
        serviceState = MQTT_SERVICE_ERROR;
    }
}

/**
 * @brief Callback quando uma publicação é recebida (cabeçalho).
 */
static void mqttIncomingPublishCallback(void *arg, const char *topic, u32_t tot_len)
{
    (void)arg;
    
    printf("[MQTT] Mensagem recebida. Topico: %.*s, Tamanho: %lu\r\n",
           (int)(sizeof(rxTopic) - 1), topic, (unsigned long)tot_len);
    
    /* Salvar informações do tópico */
    rxTopicLen = (uint16_t)strlen(topic);
    if (rxTopicLen >= sizeof(rxTopic))
    {
        rxTopicLen = sizeof(rxTopic) - 1;
    }
    memcpy(rxTopic, topic, rxTopicLen);
    rxTopic[rxTopicLen] = '\0';
    
    /* Resetar buffer de recepção */
    rxBufferPos = 0;
}

/**
 * @brief Callback quando dados de uma publicação são recebidos.
 */
static void mqttIncomingDataCallback(void *arg, const u8_t *data, 
                                      u16_t len, u8_t flags)
{
    (void)arg;
    
    /* Copiar dados para buffer */
    if (rxBufferPos + len < MQTT_RX_BUFFER_SIZE)
    {
        memcpy(&rxBuffer[rxBufferPos], data, len);
        rxBufferPos += len;
    }
    
    /* Verificar se é o último fragmento */
    if (flags & MQTT_DATA_FLAG_LAST)
    {
        /* Terminar string */
        if (rxBufferPos < MQTT_RX_BUFFER_SIZE)
        {
            rxBuffer[rxBufferPos] = '\0';
        }
        else
        {
            rxBuffer[MQTT_RX_BUFFER_SIZE - 1] = '\0';
        }
        
        printf("[MQTT] Payload: %s\r\n", (char*)rxBuffer);
        
        /* Processar mensagem */
        processReceivedMessage(rxTopic, rxTopicLen, rxBuffer, rxBufferPos);
        
        /* Atualizar estatísticas */
        if (xSemaphoreTake(mqttMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            serviceStatus.messagesReceived++;
            xSemaphoreGive(mqttMutex);
        }
    }
}

/**
 * @brief Processa uma mensagem recebida.
 */
static void processReceivedMessage(const char *topic, uint16_t topicLen,
                                   const uint8_t *payload, uint16_t payloadLen)
{
    FanCommand cmd;
    
    /* Verificar se é comando do ventilador */
    if (strncmp(topic, MQTT_TOPIC_FAN_COMMAND, topicLen) == 0)
    {
        printf("[MQTT] Comando do ventilador recebido\r\n");
        
        /* Parsear comando */
        cmd = fanControllerParseCommand((const char*)payload, payloadLen);
        
        if (cmd != FAN_CMD_INVALID)
        {
            /* Executar comando */
            fanControllerExecuteCommand(cmd);
            
            /* Publicar novo status */
            mqttServicePublishFanStatus();
        }
        else
        {
            printf("[MQTT] Comando invalido: %.*s\r\n", payloadLen, payload);
        }
    }
    
    /* Chamar callback do usuário se registrado */
    if (userCallback != NULL)
    {
        userCallback(topic, topicLen, payload, payloadLen);
    }
}

/**
 * @brief Callback de resultado da publicação.
 */
static void mqttPublishCallback(void *arg, err_t result)
{
    (void)arg;
    
    if (result != ERR_OK)
    {
        printf("[MQTT] Falha na publicacao: %d\r\n", result);
    }
}
