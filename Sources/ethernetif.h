/**
 * @file ethernetif.h
 * @brief Interface pública do driver Ethernet para LwIP no STM32F429ZI.
 *
 * Este arquivo declara as funções e tipos públicos para configuração
 * e uso do driver Ethernet com a pilha LwIP.
 */

#ifndef ETHERNETIF_H
#define ETHERNETIF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/netif.h"
#include <stdint.h>

/*-----------------------------------------------------------------------------
 * Funções Públicas
 *----------------------------------------------------------------------------*/

/**
 * @brief Inicializa a interface de rede Ethernet.
 *
 * Esta função é passada como callback para netif_add() do LwIP.
 * Configura o hardware Ethernet, inicializa os descritores DMA
 * e cria a tarefa de recepção.
 *
 * @param netif Ponteiro para a estrutura netif do LwIP.
 * @return ERR_OK se sucesso, código de erro caso contrário.
 *
 * @note Esta função não deve ser chamada diretamente.
 *       Use netif_add() passando esta função como init callback.
 *
 * @code
 * struct netif gnetif;
 * ip4_addr_t ipaddr, netmask, gw;
 * 
 * IP4_ADDR(&ipaddr, 192, 168, 1, 100);
 * IP4_ADDR(&netmask, 255, 255, 255, 0);
 * IP4_ADDR(&gw, 192, 168, 1, 1);
 * 
 * netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, ethernetIfInit, tcpip_input);
 * @endcode
 */
err_t ethernetIfInit(struct netif *netif);

/**
 * @brief Configura o endereço MAC da interface.
 *
 * Permite definir um endereço MAC customizado para a interface
 * Ethernet. Deve ser chamado antes de netif_add() ou requer
 * reinicialização da interface para ter efeito completo.
 *
 * @param macAddr Array de 6 bytes com o endereço MAC.
 *                Formato: {0x02, 0x00, 0x00, 0x00, 0x00, 0x01}
 *
 * @note O primeiro byte deve ter o bit LSB = 0 para endereço unicast
 *       e bit 1 = 1 para endereço localmente administrado.
 */
void ethernetIfSetMacAddress(const uint8_t *macAddr);

/**
 * @brief Retorna o estado do link Ethernet.
 *
 * @return 1 se o link está ativo (cabo conectado e negociação OK).
 * @return 0 se o link está inativo (cabo desconectado ou falha).
 */
uint8_t ethernetIfGetLinkState(void);

#ifdef __cplusplus
}
#endif

#endif /* ETHERNETIF_H */
