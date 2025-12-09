/**
 * Implementação básica do dispatcher REST.
 *
 * Este arquivo é um stub inicial. A integração completa com restEndpoints[]
 * e geração de respostas HTTP deve ser implementada conforme a necessidade
 * da aplicação.
 */

#include <string.h>
#include "rest_dispatcher.h"

int restDispatch(const char *uri,
                 const char *method,
                 const char *body,
                 unsigned int bodyLength,
                 struct fs_file *fileOut)
{
    (void)uri;
    (void)method;
    (void)body;
    (void)bodyLength;
    (void)fileOut;

    /* TODO: implementar roteamento usando restEndpoints[] e montar resposta */
    return 0;
}
