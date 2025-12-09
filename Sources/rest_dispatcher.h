/**
 * Dispatcher REST para o servidor HTTP baseado em LwIP.
 *
 * Este arquivo foi criado inicialmente como stub. Pode ser editado manualmente
 * para integrar com a lógica de negócio do firmware.
 */

#ifndef REST_DISPATCHER_H
#define REST_DISPATCHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/apps/fs.h"
#include "rest_endpoints.h"

/**
 * Despacha uma requisição HTTP para os handlers REST registrados em
 * restEndpoints[].
 *
 * Retorno:
 *  - diferente de zero: requisição atendida e resposta preenchida em fileOut.
 *  - zero: rota não tratada pelo dispatcher REST.
 */
int restDispatch(const char *uri,
                 const char *method,
                 const char *body,
                 unsigned int bodyLength,
                 struct fs_file *fileOut);

#ifdef __cplusplus
}
#endif

#endif /* REST_DISPATCHER_H */
