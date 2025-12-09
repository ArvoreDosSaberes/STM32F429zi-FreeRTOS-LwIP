/**
 * Implementações básicas (stubs) dos handlers REST gerados a partir do Swagger.
 *
 * As funções abaixo apenas retornam 0 por enquanto. A lógica real de acesso
 * a sensores, atuadores e métricas (temperatura, ventilador, uptime, etc.)
 * deve ser implementada conforme a necessidade do firmware.
 */

#include "rest_endpoints.h"
#include <stdint.h>
#include <stddef.h>

static uint32_t lastPageLoadTimeMs = 0U;

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

int restHandle_POST_api_v1_page_load(const restRequestContext_t *ctx)
{
    uint32_t value = 0U;
    unsigned int i;

    if ((ctx == NULL) || (ctx->body == NULL) || (ctx->bodyLength == 0U))
    {
        return 0;
    }

    for (i = 0U; i < ctx->bodyLength; i++)
    {
        char c = ctx->body[i];

        if ((c >= '0') && (c <= '9'))
        {
            value = (value * 10U) + (uint32_t)(c - '0');
        }
        else
        {
            break;
        }
    }

    lastPageLoadTimeMs = value;

    return 0;
}
