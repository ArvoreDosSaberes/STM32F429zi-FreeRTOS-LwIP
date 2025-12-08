#ifndef OPCUA_SERVER_H
#define OPCUA_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Porta padrão do servidor OPC-UA.
 */
#define OPCUA_SERVER_PORT                4840

/**
 * @brief Número máximo de conexões simultâneas.
 */
#define OPCUA_MAX_CONNECTIONS           2

/**
 * @brief Intervalo de atualização do servidor OPC-UA em milissegundos.
 */
#define OPCUA_UPDATE_INTERVAL_MS        1000

/**
 * @brief Namespace index utilizado para os nós do dispositivo.
 */
#define OPCUA_NAMESPACE_INDEX           1

/**
 * @brief Callback para comandos de ventilador recebidos via OPC-UA.
 *
 * @param on true para ligar o ventilador, false para desligar.
 */
typedef void (*OpcuaFanCommandCallback)(bool on);

/**
 * @brief Inicializa a infraestrutura do servidor OPC-UA.
 *
 * Esta função configura o servidor open62541, registra o namespace
 * do dispositivo e cria os nós de variável necessários:
 * - ns=1;i=1001: Temperature (Float, read-only)
 * - ns=1;i=1002: FanState (Boolean, read-only)
 * - ns=1;i=1003: FanCommand (Boolean, write)
 * - ns=1;i=1004: TempMax (Float, read/write)
 * - ns=1;i=1005: TempMin (Float, read/write)
 *
 * @return 0 em caso de sucesso, valor negativo em caso de erro.
 */
int opcuaServerInit(void);

/**
 * @brief Define o callback para comandos de ventilador.
 *
 * Quando o cliente OPC-UA escreve no nó FanCommand (ns=1;i=1003),
 * o servidor chamará este callback com o estado desejado.
 *
 * @param callback Função a ser chamada quando houver comando de ventilador.
 */
void opcuaServerSetFanCallback(OpcuaFanCommandCallback callback);

/**
 * @brief Inicia a tarefa FreeRTOS responsável pelo servidor OPC-UA.
 *
 * Esta função cria uma tarefa de baixa prioridade que executa
 * periodicamente o loop do servidor OPC-UA (UA_Server_run_iterate)
 * e atualiza as variáveis de temperatura e estado do ventilador.
 *
 * @return 0 em caso de sucesso, valor negativo em caso de erro.
 */
int opcuaServerStart(void);

#ifdef __cplusplus
}
#endif

#endif /* OPCUA_SERVER_H */
