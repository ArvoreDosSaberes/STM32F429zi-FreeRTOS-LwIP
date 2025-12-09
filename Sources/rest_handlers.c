/**
 * Implementações básicas (stubs) dos handlers REST gerados a partir do Swagger.
 *
 * As funções abaixo apenas retornam 0 por enquanto. A lógica real de acesso
 * a sensores, atuadores e métricas (temperatura, ventilador, uptime, etc.)
 * deve ser implementada conforme a necessidade do firmware.
 */

#include "rest_endpoints.h"

int restHandle_GET_api_v1_temperature(const restRequestContext_t *ctx)
{
    (void)ctx;
    return 0;
}

int restHandle_GET_api_v1_fan(const restRequestContext_t *ctx)
{
    (void)ctx;
    return 0;
}

int restHandle_POST_api_v1_fan(const restRequestContext_t *ctx)
{
    (void)ctx;
    return 0;
}

int restHandle_GET_api_v1_time(const restRequestContext_t *ctx)
{
    (void)ctx;
    return 0;
}
