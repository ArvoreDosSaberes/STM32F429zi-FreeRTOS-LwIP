/**
 * Arquivo gerado automaticamente por swagger2rest.py.
 * NÃO EDITE MANUALMENTE: alterações serão sobrescritas.
 */

#ifndef REST_ENDPOINTS_H
#define REST_ENDPOINTS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_PATCH,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_TRACE,
    HTTP_METHOD_CONNECT,
} httpMethod_t;

typedef struct restRequestContext {
    const char   *uri;
    const char   *queryString;
    const char   *body;
    unsigned int  bodyLength;
    httpMethod_t  method;
} restRequestContext_t;

typedef int (*restHandlerFn)(const restRequestContext_t *ctx);

typedef struct restEndpoint {
    const char   *pathPattern;
    httpMethod_t  method;
    restHandlerFn handler;
} restEndpoint_t;

int restHandle_GET_api_v1_temperature(const restRequestContext_t *ctx);
int restHandle_GET_api_v1_fan(const restRequestContext_t *ctx);
int restHandle_POST_api_v1_fan(const restRequestContext_t *ctx);
int restHandle_GET_api_v1_time(const restRequestContext_t *ctx);
int restHandle_POST_api_v1_page_load(const restRequestContext_t *ctx);

static const restEndpoint_t restEndpoints[] = {
    { "/api/v1/temperature", HTTP_METHOD_GET, restHandle_GET_api_v1_temperature },
    { "/api/v1/fan", HTTP_METHOD_GET, restHandle_GET_api_v1_fan },
    { "/api/v1/fan", HTTP_METHOD_POST, restHandle_POST_api_v1_fan },
    { "/api/v1/time", HTTP_METHOD_GET, restHandle_GET_api_v1_time },
    { "/api/v1/page-load", HTTP_METHOD_POST, restHandle_POST_api_v1_page_load },
};

static const unsigned int restEndpointCount = sizeof(restEndpoints) / sizeof(restEndpoints[0]);

#ifdef __cplusplus
}
#endif

#endif /* REST_ENDPOINTS_H */
