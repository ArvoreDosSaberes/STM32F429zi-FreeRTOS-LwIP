/**
 * @file httpserver.h
 * @brief Interface pública do servidor HTTP para STM32F429ZI.
 *
 * Este arquivo declara as funções públicas para inicialização e
 * controle do servidor HTTP integrado.
 */

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/netif.h"

/*-----------------------------------------------------------------------------
 * Funções Públicas
 *----------------------------------------------------------------------------*/

/**
 * @brief Inicializa o stack de rede e o servidor HTTP.
 *
 * Esta função configura a pilha TCP/IP LwIP, adiciona a interface
 * Ethernet, inicia o cliente DHCP e inicializa o servidor HTTP
 * na porta 80.
 *
 * @return 0 se sucesso, -1 se erro na inicialização.
 *
 * @note Esta função deve ser chamada após vTaskStartScheduler()
 *       ou dentro de uma tarefa FreeRTOS.
 *
 * @code
 * void mainTask(void *pvParameters)
 * {
 *     if (httpServerInit() != 0)
 *     {
 *         // Erro de inicialização
 *         while(1);
 *     }
 *     
 *     // Servidor HTTP rodando...
 *     for(;;)
 *     {
 *         vTaskDelay(pdMS_TO_TICKS(1000));
 *     }
 * }
 * @endcode
 */
int httpServerInit(void);

/**
 * @brief Retorna o endereço IP atual da interface.
 *
 * @param ipStr Buffer para armazenar o IP em formato string.
 *              Deve ter no mínimo 16 bytes.
 * @return 1 se IP válido foi obtido e copiado para o buffer.
 * @return 0 se não há IP válido (DHCP em andamento ou falha).
 *
 * @code
 * char ipBuffer[16];
 * if (httpServerGetIpAddress(ipBuffer))
 * {
 *     printf("Server IP: %s\n", ipBuffer);
 * }
 * @endcode
 */
int httpServerGetIpAddress(char *ipStr);

/**
 * @brief Retorna a estrutura netif da interface HTTP.
 *
 * @return Ponteiro para a estrutura netif do LwIP.
 *
 * @note Útil para acessar informações de baixo nível da interface.
 */
struct netif *httpServerGetNetif(void);

/**
 * @brief Verifica se a rede está inicializada e conectada.
 *
 * @return 1 se a interface está UP e o link está ativo.
 * @return 0 se a interface está DOWN ou sem link.
 */
int httpServerIsConnected(void);

#ifdef __cplusplus
}
#endif

#endif /* HTTPSERVER_H */
