/**
 * @file mqtt_config.h
 * @brief Configuração do cliente MQTT para STM32F429ZI.
 * 
 * Este arquivo valida e organiza os parâmetros de conexão MQTT que são
 * injetados em tempo de compilação via CMake/env.cmake.
 * 
 * IMPORTANTE: As variáveis são definidas pelo CMake a partir do env.cmake.
 *             Não há valores padrão aqui (exceto LOG_LEVEL, MQTT_BROKER e
 *             ENABLE_STACK_WATERMARK). Se uma variável obrigatória não for
 *             definida, a compilação falhará com um #error.
 * 
 * @note Configure as variáveis no arquivo env.cmake (copie de env.cmake.example).
 */

#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------
 * Validação das Variáveis Obrigatórias (definidas pelo CMake)
 *----------------------------------------------------------------------------*/

/**
 * @brief Endereço do broker MQTT.
 *        Valor padrão permitido: mqtt.rapport.tec.br
 */
#ifndef MQTT_BROKER
    #define MQTT_BROKER             "mqtt.rapport.tec.br"
#endif

/**
 * @brief Porta do broker MQTT.
 *        OBRIGATÓRIO: deve ser definido no env.cmake.
 */
#ifndef MQTT_BROKER_PORT
    #error "MQTT_BROKER_PORT nao definido! Configure no env.cmake (ex: 1883 ou 8883)"
#endif

/**
 * @brief Identificador único do cliente MQTT.
 *        OBRIGATÓRIO: deve ser definido no env.cmake.
 */
#ifndef MQTT_CLIENT_ID
    #error "MQTT_CLIENT_ID nao definido! Configure no env.cmake"
#endif

/**
 * @brief Tópico base para publicação e subscrição.
 *        OBRIGATÓRIO: deve ser definido no env.cmake.
 */
#ifndef MQTT_BASE_TOPIC
    #error "MQTT_BASE_TOPIC nao definido! Configure no env.cmake"
#endif

/**
 * @brief Identificador do rack/dispositivo.
 *        OBRIGATÓRIO: deve ser definido no env.cmake.
 */
#ifndef MQTT_RACK_NUMBER
    #error "MQTT_RACK_NUMBER nao definido! Configure no env.cmake"
#endif

/*-----------------------------------------------------------------------------
 * Variáveis Opcionais (autenticação MQTT)
 * MQTT_USERNAME e MQTT_PASSWORD podem ser vazios ("") ou não definidos.
 *----------------------------------------------------------------------------*/

/**
 * @brief Usuário para autenticação no broker MQTT.
 *        Opcional: pode ser "" se o broker não requer autenticação.
 */
#ifndef MQTT_USERNAME
    #define MQTT_USERNAME           ""
#endif

/**
 * @brief Senha para autenticação no broker MQTT.
 *        Opcional: pode ser "" se o broker não requer autenticação.
 */
#ifndef MQTT_PASSWORD
    #define MQTT_PASSWORD           ""
#endif

/*-----------------------------------------------------------------------------
 * Variáveis com Valores Padrão Permitidos
 *----------------------------------------------------------------------------*/

/**
 * @brief Nível de log do sistema.
 *        Valor padrão: 1 (ERROR)
 *        Níveis: 0=OFF, 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG, 5=VERBOSE
 */
#ifndef LOG_LEVEL
    #define LOG_LEVEL               1
#endif

/**
 * @brief Habilita monitoramento de stack watermark.
 *        Valor padrão: 0 (desabilitado)
 */
#ifndef ENABLE_STACK_WATERMARK
    #define ENABLE_STACK_WATERMARK  0
#endif

/*-----------------------------------------------------------------------------
 * Configuração de Comportamento MQTT
 * Validação de variáveis que devem vir do env.cmake
 *----------------------------------------------------------------------------*/

/**
 * @brief Intervalo de publicação da temperatura em milissegundos.
 *        OBRIGATÓRIO: deve ser definido no env.cmake.
 */
#ifndef MQTT_PUBLISH_INTERVAL_MS
    #error "MQTT_PUBLISH_INTERVAL_MS nao definido! Configure no env.cmake"
#endif

/**
 * @brief Quality of Service para mensagens publicadas.
 *        OBRIGATÓRIO: deve ser definido no env.cmake.
 *        Valores: 0, 1 ou 2
 */
#ifndef MQTT_QOS
    #error "MQTT_QOS nao definido! Configure no env.cmake (0, 1 ou 2)"
#endif

/**
 * @brief Keep-alive interval em segundos.
 *        OBRIGATÓRIO: deve ser definido no env.cmake.
 */
#ifndef MQTT_KEEP_ALIVE_S
    #error "MQTT_KEEP_ALIVE_S nao definido! Configure no env.cmake"
#endif

/*-----------------------------------------------------------------------------
 * Constantes Derivadas (calculadas a partir das variáveis acima)
 *----------------------------------------------------------------------------*/

/**
 * @brief Tópico para publicar a temperatura.
 *        Formato: <base>/<rack_number>/temperature
 */
#define MQTT_TOPIC_TEMPERATURE      MQTT_BASE_TOPIC "/" MQTT_RACK_NUMBER "/temperature"

/**
 * @brief Tópico para receber comandos do ventilador.
 *        Formato: <base>/<rack_number>/fan/command
 */
#define MQTT_TOPIC_FAN_COMMAND      MQTT_BASE_TOPIC "/" MQTT_RACK_NUMBER "/fan/command"

/**
 * @brief Tópico para publicar status do ventilador.
 *        Formato: <base>/<rack_number>/fan/status
 */
#define MQTT_TOPIC_FAN_STATUS       MQTT_BASE_TOPIC "/" MQTT_RACK_NUMBER "/fan/status"

/**
 * @brief Tópico para status geral do dispositivo.
 *        Formato: <base>/<rack_number>/status
 */
#define MQTT_TOPIC_STATUS           MQTT_BASE_TOPIC "/" MQTT_RACK_NUMBER "/status"

/*-----------------------------------------------------------------------------
 * Configurações Fixas do Sistema (não configuráveis via env.cmake)
 *----------------------------------------------------------------------------*/

/**
 * @brief Flag de retenção para mensagens publicadas.
 *        0 = Não reter no broker
 *        1 = Reter última mensagem no broker
 */
#define MQTT_RETAIN                 0

/**
 * @brief Timeout de conexão em milissegundos.
 */
#define MQTT_CONNECT_TIMEOUT_MS     10000

/**
 * @brief Intervalo de reconexão após falha em milissegundos.
 */
#define MQTT_RECONNECT_INTERVAL_MS  5000

/**
 * @brief Número máximo de tentativas de reconexão.
 *        0 = Tentar indefinidamente.
 */
#define MQTT_MAX_RECONNECT_ATTEMPTS 0

/*-----------------------------------------------------------------------------
 * Configuração do LwIP MQTT (constantes fixas)
 *----------------------------------------------------------------------------*/

/**
 * @brief Tamanho do buffer de saída MQTT em bytes.
 */
#define MQTT_OUTPUT_RINGBUF_SIZE_CFG    256

/**
 * @brief Tamanho máximo da mensagem MQTT em bytes.
 */
#define MQTT_VAR_HEADER_BUFFER_LEN_CFG  128

#ifdef __cplusplus
}
#endif

#endif /* MQTT_CONFIG_H */
