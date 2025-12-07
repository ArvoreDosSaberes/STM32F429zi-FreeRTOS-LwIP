/**
 * @file webpages.h
 * @brief Interface pública para páginas web embarcadas.
 *
 * Este arquivo declara as funções para registro e acesso às
 * páginas HTML servidas pelo servidor HTTP.
 */

#ifndef WEBPAGES_H
#define WEBPAGES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*-----------------------------------------------------------------------------
 * Funções Públicas
 *----------------------------------------------------------------------------*/

/**
 * @brief Registra as páginas web no servidor HTTP do LwIP.
 *
 * Esta função deve ser chamada após httpd_init() para configurar
 * o sistema de arquivos customizado.
 *
 * @note Deve ser chamada apenas uma vez durante a inicialização.
 */
void webpagesRegister(void);

/**
 * @brief Retorna os dados da landing page (index.html).
 *
 * @param len Ponteiro para armazenar o tamanho dos dados em bytes.
 *            Pode ser NULL se o tamanho não for necessário.
 * @return Ponteiro para os dados HTML em flash.
 *
 * @note Os dados retornados são read-only e residem em flash.
 */
const char *webpagesGetIndex(uint32_t *len);

/**
 * @brief Retorna os dados da página de erro 404.
 *
 * @param len Ponteiro para armazenar o tamanho dos dados em bytes.
 *            Pode ser NULL se o tamanho não for necessário.
 * @return Ponteiro para os dados HTML em flash.
 *
 * @note Os dados retornados são read-only e residem em flash.
 */
const char *webpagesGet404(uint32_t *len);

#ifdef __cplusplus
}
#endif

#endif /* WEBPAGES_H */
