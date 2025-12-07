/**
 * @file mqtt_service.h
 * @brief Interface do serviço MQTT para STM32F429ZI com FreeRTOS.
 * 
 * Este serviço gerencia a conexão MQTT, publicação de temperatura e
 * subscrição para comandos do ventilador. Utiliza o cliente MQTT do LwIP
 * e tarefas FreeRTOS para operação assíncrona.
 * 
 * Funcionalidades:
 * - Conexão automática ao broker MQTT com reconexão
 * - Publicação periódica da temperatura do chip
 * - Subscrição ao tópico de comandos do ventilador
 * - Callbacks para processamento de mensagens recebidas
 * 
 * @note Requer LwIP com cliente MQTT habilitado e FreeRTOS.
 */

#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "mqtt_config.h"

/*-----------------------------------------------------------------------------
 * Estados do Serviço MQTT
 *----------------------------------------------------------------------------*/

/**
 * @brief Estados possíveis do serviço MQTT.
 */
typedef enum {
    MQTT_SERVICE_DISCONNECTED   = 0,    /**< Desconectado do broker */
    MQTT_SERVICE_CONNECTING     = 1,    /**< Tentando conectar */
    MQTT_SERVICE_CONNECTED      = 2,    /**< Conectado ao broker */
    MQTT_SERVICE_SUBSCRIBING    = 3,    /**< Inscrevendo nos tópicos */
    MQTT_SERVICE_READY          = 4,    /**< Pronto para operar */
    MQTT_SERVICE_ERROR          = -1    /**< Erro no serviço */
} MqttServiceState;

/**
 * @brief Códigos de erro do serviço MQTT.
 */
typedef enum {
    MQTT_SERVICE_OK             = 0,    /**< Operação bem-sucedida */
    MQTT_SERVICE_ERR_INIT       = -1,   /**< Falha na inicialização */
    MQTT_SERVICE_ERR_NETWORK    = -2,   /**< Erro de rede */
    MQTT_SERVICE_ERR_CONNECT    = -3,   /**< Falha na conexão */
    MQTT_SERVICE_ERR_SUBSCRIBE  = -4,   /**< Falha na subscrição */
    MQTT_SERVICE_ERR_PUBLISH    = -5,   /**< Falha na publicação */
    MQTT_SERVICE_ERR_DNS        = -6,   /**< Falha na resolução DNS */
    MQTT_SERVICE_ERR_TIMEOUT    = -7    /**< Timeout */
} MqttServiceError;

/*-----------------------------------------------------------------------------
 * Estruturas de Dados
 *----------------------------------------------------------------------------*/

/**
 * @brief Estrutura com informações de status do serviço MQTT.
 */
typedef struct {
    MqttServiceState state;             /**< Estado atual do serviço */
    uint32_t messagesPublished;         /**< Total de mensagens publicadas */
    uint32_t messagesReceived;          /**< Total de mensagens recebidas */
    uint32_t reconnectCount;            /**< Número de reconexões */
    float lastTemperature;              /**< Última temperatura publicada */
    bool fanState;                      /**< Estado atual do ventilador */
} MqttServiceStatus;

/**
 * @brief Callback para mensagens recebidas.
 * 
 * @param topic Tópico da mensagem (pode não ser null-terminated).
 * @param topicLen Tamanho do tópico.
 * @param payload Payload da mensagem (pode não ser null-terminated).
 * @param payloadLen Tamanho do payload.
 */
typedef void (*MqttMessageCallback)(const char *topic, uint16_t topicLen,
                                     const uint8_t *payload, uint16_t payloadLen);

/*-----------------------------------------------------------------------------
 * Funções Públicas - Inicialização e Controle
 *----------------------------------------------------------------------------*/

/**
 * @brief Inicializa o serviço MQTT.
 * 
 * Inicializa os módulos de sensor de temperatura e controlador do ventilador,
 * e prepara o cliente MQTT para conexão. Não inicia a conexão automaticamente.
 * 
 * @return MQTT_SERVICE_OK em caso de sucesso, código de erro caso contrário.
 * 
 * @note Deve ser chamado após a inicialização da rede (LwIP + DHCP).
 */
MqttServiceError mqttServiceInit(void);

/**
 * @brief Inicia a tarefa principal do serviço MQTT.
 * 
 * Cria a tarefa FreeRTOS que gerencia a conexão, publicações e subscrições.
 * A tarefa opera de forma autônoma, reconectando automaticamente se necessário.
 * 
 * @return MQTT_SERVICE_OK em caso de sucesso, código de erro caso contrário.
 * 
 * @note A tarefa utiliza prioridade média e stack de 1024 palavras.
 */
MqttServiceError mqttServiceStart(void);

/**
 * @brief Para o serviço MQTT.
 * 
 * Desconecta do broker e deleta a tarefa FreeRTOS.
 */
void mqttServiceStop(void);

/**
 * @brief Verifica se o serviço está conectado ao broker.
 * 
 * @return true se conectado e pronto, false caso contrário.
 */
bool mqttServiceIsConnected(void);

/**
 * @brief Obtém o estado atual do serviço.
 * 
 * @return Estado atual do serviço MQTT.
 */
MqttServiceState mqttServiceGetState(void);

/**
 * @brief Obtém estatísticas e status do serviço.
 * 
 * @param[out] status Ponteiro para estrutura de status.
 */
void mqttServiceGetStatus(MqttServiceStatus *status);

/*-----------------------------------------------------------------------------
 * Funções Públicas - Publicação
 *----------------------------------------------------------------------------*/

/**
 * @brief Força publicação imediata da temperatura.
 * 
 * Lê a temperatura atual e publica no tópico configurado.
 * Útil para publicação sob demanda fora do intervalo regular.
 * 
 * @return MQTT_SERVICE_OK em caso de sucesso, código de erro caso contrário.
 */
MqttServiceError mqttServicePublishTemperature(void);

/**
 * @brief Publica o status atual do ventilador.
 * 
 * @return MQTT_SERVICE_OK em caso de sucesso, código de erro caso contrário.
 */
MqttServiceError mqttServicePublishFanStatus(void);

/**
 * @brief Publica uma mensagem em um tópico específico.
 * 
 * @param topic Tópico para publicação (null-terminated).
 * @param payload Payload da mensagem.
 * @param payloadLen Tamanho do payload.
 * @param qos Quality of Service (0, 1 ou 2).
 * @param retain Flag de retenção.
 * 
 * @return MQTT_SERVICE_OK em caso de sucesso, código de erro caso contrário.
 */
MqttServiceError mqttServicePublish(const char *topic, const void *payload,
                                     uint16_t payloadLen, uint8_t qos, bool retain);

/*-----------------------------------------------------------------------------
 * Funções Públicas - Callbacks
 *----------------------------------------------------------------------------*/

/**
 * @brief Registra callback para mensagens recebidas.
 * 
 * O callback será invocado para todas as mensagens recebidas,
 * além do processamento interno (comandos do ventilador).
 * 
 * @param callback Função de callback (NULL para remover).
 */
void mqttServiceSetMessageCallback(MqttMessageCallback callback);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_SERVICE_H */
