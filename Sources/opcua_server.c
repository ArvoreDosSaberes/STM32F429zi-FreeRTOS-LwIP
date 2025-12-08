#include "opcua_server.h"

#include "FreeRTOS.h"
#include "task.h"

#include "httpserver.h"
#include "temperature_sensor.h"
#include "fan_controller.h"

#include "open62541.h"

#include <stdio.h>

/*----------------------------------------------------------------------------- 
 * Variáveis Estáticas
 *----------------------------------------------------------------------------*/

static UA_Server *opcuaServer = NULL;
static OpcuaFanCommandCallback fanCommandCallback = NULL;
static bool opcuaInitialized = false;
static bool opcuaTaskStarted = false;

static float tempMax = 45.0f;
static float tempMin = 20.0f;

/* NodeIds fixos conforme documentação/opcua_dashboard.py */
#define NODEID_TEMPERATURE   UA_NODEID_NUMERIC(OPCUA_NAMESPACE_INDEX, 1001)
#define NODEID_FAN_STATE     UA_NODEID_NUMERIC(OPCUA_NAMESPACE_INDEX, 1002)
#define NODEID_FAN_COMMAND   UA_NODEID_NUMERIC(OPCUA_NAMESPACE_INDEX, 1003)
#define NODEID_TEMP_MAX      UA_NODEID_NUMERIC(OPCUA_NAMESPACE_INDEX, 1004)
#define NODEID_TEMP_MIN      UA_NODEID_NUMERIC(OPCUA_NAMESPACE_INDEX, 1005)

/*----------------------------------------------------------------------------- 
 * Prototipos
 *----------------------------------------------------------------------------*/

static void opcuaServerTask(void *pvParameters);
static UA_StatusCode writeFanCommand(UA_Server *server,
                                     const UA_NodeId *sessionId, void *sessionContext,
                                     const UA_NodeId *nodeId, void *nodeContext,
                                     const UA_NumericRange *range, const UA_DataValue *data);
static UA_StatusCode writeTempMax(UA_Server *server,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *nodeId, void *nodeContext,
                                  const UA_NumericRange *range, const UA_DataValue *data);
static UA_StatusCode writeTempMin(UA_Server *server,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *nodeId, void *nodeContext,
                                  const UA_NumericRange *range, const UA_DataValue *data);
static void updateTemperatureNode(void);
static void updateFanStateNode(void);

/*----------------------------------------------------------------------------- 
 * Implementação Pública
 *----------------------------------------------------------------------------*/

int opcuaServerInit(void)
{
    if (opcuaInitialized)
    {
        return 0;
    }

    /* Inicializar sensores/atuadores necessários */
    if (!temperatureSensorIsInitialized())
    {
        if (temperatureSensorInit() != TEMP_SENSOR_OK)
        {
            printf("[OPCUA] Erro ao inicializar sensor de temperatura\r\n");
            return -1;
        }
    }

    /* fanControllerInit é idempotente o suficiente para ser chamado aqui */
    if (fanControllerInit() != 0)
    {
        printf("[OPCUA] Erro ao inicializar controlador do ventilador\r\n");
        return -2;
    }

    /* Criar instância do servidor e configuração mínima padrão */
    opcuaServer = UA_Server_new();
    if (opcuaServer == NULL)
    {
        printf("[OPCUA] Erro ao criar instancia do servidor\r\n");
        return -3;
    }

    UA_ServerConfig *config = UA_Server_getConfig(opcuaServer);
    UA_StatusCode cfgStatus = UA_ServerConfig_setMinimal(config, OPCUA_SERVER_PORT, NULL);
    if (cfgStatus != UA_STATUSCODE_GOOD)
    {
        printf("[OPCUA] Erro ao configurar servidor OPC-UA: 0x%08lX\r\n", (unsigned long)cfgStatus);
        return -4;
    }

    /* Registrar namespace do dispositivo (ns=1) */
    UA_UInt16 nsIndex = UA_Server_addNamespace(opcuaServer, "STM32F429");
    (void)nsIndex; /* OPCUA_NAMESPACE_INDEX já é 1 por contrato */

    /* Criar nós de variáveis */
    UA_VariableAttributes attr;

    /* Temperature (Float, read-only) */
    UA_VariableAttributes_init(&attr);
    float initialTemp = 0.0f;
    UA_Variant_setScalar(&attr.value, &initialTemp, &UA_TYPES[UA_TYPES_FLOAT]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Temperature");
    attr.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;

    UA_StatusCode ret = UA_Server_addVariableNode(
        opcuaServer,
        NODEID_TEMPERATURE,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(OPCUA_NAMESPACE_INDEX, "Temperature"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    if (ret != UA_STATUSCODE_GOOD)
    {
        printf("[OPCUA] Erro ao criar no Temperature: 0x%08lX\r\n", (unsigned long)ret);
        return -5;
    }

    /* FanState (Boolean, read-only) */
    UA_VariableAttributes_init(&attr);
    UA_Boolean fanInitial = (UA_Boolean)fanControllerIsOn();
    UA_Variant_setScalar(&attr.value, &fanInitial, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "FanState");
    attr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;

    ret = UA_Server_addVariableNode(
        opcuaServer,
        NODEID_FAN_STATE,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(OPCUA_NAMESPACE_INDEX, "FanState"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    if (ret != UA_STATUSCODE_GOOD)
    {
        printf("[OPCUA] Erro ao criar no FanState: 0x%08lX\r\n", (unsigned long)ret);
        return -6;
    }

    /* FanCommand (Boolean, write) */
    UA_VariableAttributes_init(&attr);
    UA_Boolean cmdInitial = (UA_Boolean)fanControllerIsOn();
    UA_Variant_setScalar(&attr.value, &cmdInitial, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "FanCommand");
    attr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    ret = UA_Server_addVariableNode(
        opcuaServer,
        NODEID_FAN_COMMAND,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(OPCUA_NAMESPACE_INDEX, "FanCommand"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    if (ret != UA_STATUSCODE_GOOD)
    {
        printf("[OPCUA] Erro ao criar no FanCommand: 0x%08lX\r\n", (unsigned long)ret);
        return -7;
    }

    /* Registrar callback de escrita para FanCommand */
    UA_DataSource fanCmdDs;
    fanCmdDs.read = NULL; /* leitura padrão a partir do valor armazenado */
    fanCmdDs.write = writeFanCommand;
    ret = UA_Server_setVariableNode_dataSource(opcuaServer, NODEID_FAN_COMMAND, fanCmdDs);
    if (ret != UA_STATUSCODE_GOOD)
    {
        printf("[OPCUA] Erro ao registrar DataSource FanCommand: 0x%08lX\r\n", (unsigned long)ret);
        return -8;
    }

    /* TempMax (Float, read/write) */
    UA_VariableAttributes_init(&attr);
    UA_Variant_setScalar(&attr.value, &tempMax, &UA_TYPES[UA_TYPES_FLOAT]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TempMax");
    attr.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    ret = UA_Server_addVariableNode(
        opcuaServer,
        NODEID_TEMP_MAX,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(OPCUA_NAMESPACE_INDEX, "TempMax"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    if (ret != UA_STATUSCODE_GOOD)
    {
        printf("[OPCUA] Erro ao criar no TempMax: 0x%08lX\r\n", (unsigned long)ret);
        return -9;
    }

    UA_DataSource tempMaxDs;
    tempMaxDs.read = NULL;
    tempMaxDs.write = writeTempMax;
    ret = UA_Server_setVariableNode_dataSource(opcuaServer, NODEID_TEMP_MAX, tempMaxDs);
    if (ret != UA_STATUSCODE_GOOD)
    {
        printf("[OPCUA] Erro ao registrar DataSource TempMax: 0x%08lX\r\n", (unsigned long)ret);
        return -10;
    }

    /* TempMin (Float, read/write) */
    UA_VariableAttributes_init(&attr);
    UA_Variant_setScalar(&attr.value, &tempMin, &UA_TYPES[UA_TYPES_FLOAT]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "TempMin");
    attr.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    ret = UA_Server_addVariableNode(
        opcuaServer,
        NODEID_TEMP_MIN,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(OPCUA_NAMESPACE_INDEX, "TempMin"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    if (ret != UA_STATUSCODE_GOOD)
    {
        printf("[OPCUA] Erro ao criar no TempMin: 0x%08lX\r\n", (unsigned long)ret);
        return -11;
    }

    UA_DataSource tempMinDs;
    tempMinDs.read = NULL;
    tempMinDs.write = writeTempMin;
    ret = UA_Server_setVariableNode_dataSource(opcuaServer, NODEID_TEMP_MIN, tempMinDs);
    if (ret != UA_STATUSCODE_GOOD)
    {
        printf("[OPCUA] Erro ao registrar DataSource TempMin: 0x%08lX\r\n", (unsigned long)ret);
        return -12;
    }

    opcuaInitialized = true;
    printf("[OPCUA] Servidor inicializado na porta %d\r\n", OPCUA_SERVER_PORT);

    return 0;
}

void opcuaServerSetFanCallback(OpcuaFanCommandCallback callback)
{
    fanCommandCallback = callback;
}

int opcuaServerStart(void)
{
    if (!opcuaInitialized || opcuaServer == NULL)
    {
        return -1;
    }

    if (opcuaTaskStarted)
    {
        return 0;
    }

    BaseType_t res = xTaskCreate(opcuaServerTask,
                                 "OPCUA",
                                 1024,
                                 NULL,
                                 2,
                                 NULL);
    if (res != pdPASS)
    {
        printf("[OPCUA] Erro ao criar tarefa OPC-UA\r\n");
        return -2;
    }

    opcuaTaskStarted = true;
    return 0;
}

/*----------------------------------------------------------------------------- 
 * Implementação Privada
 *----------------------------------------------------------------------------*/

static void opcuaServerTask(void *pvParameters)
{
    (void)pvParameters;

    printf("[OPCUA] Tarefa OPC-UA iniciada\r\n");

    /* Aguardar rede disponível */
    while (!httpServerIsConnected())
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    printf("[OPCUA] Rede disponivel, iniciando loop do servidor OPC-UA\r\n");

    for (;;)
    {
        updateTemperatureNode();
        updateFanStateNode();

        /* Executar iteração do servidor */
        UA_Server_run_iterate(opcuaServer, false);

        vTaskDelay(pdMS_TO_TICKS(OPCUA_UPDATE_INTERVAL_MS));
    }
}

static void updateTemperatureNode(void)
{
    float temp;
    if (temperatureSensorRead(&temp) == TEMP_SENSOR_OK)
    {
        UA_Variant value;
        UA_Variant_init(&value);
        UA_Variant_setScalar(&value, &temp, &UA_TYPES[UA_TYPES_FLOAT]);
        UA_Server_writeValue(opcuaServer, NODEID_TEMPERATURE, value);
    }
}

static void updateFanStateNode(void)
{
    UA_Boolean fan = (UA_Boolean)fanControllerIsOn();
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &fan, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(opcuaServer, NODEID_FAN_STATE, value);
}

static UA_StatusCode writeFanCommand(UA_Server *server,
                                     const UA_NodeId *sessionId, void *sessionContext,
                                     const UA_NodeId *nodeId, void *nodeContext,
                                     const UA_NumericRange *range, const UA_DataValue *data)
{
    (void)server;
    (void)sessionId;
    (void)sessionContext;
    (void)nodeId;
    (void)nodeContext;
    (void)range;

    if (!data || !UA_Variant_isScalar(&data->value))
    {
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    UA_Boolean *val = (UA_Boolean*)data->value.data;
    if (val == NULL)
    {
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    bool on = (*val != UA_FALSE);

    if (fanCommandCallback)
    {
        fanCommandCallback(on);
    }
    else
    {
        fanControllerSetState(on ? FAN_STATE_ON : FAN_STATE_OFF);
    }

    /* Atualizar nós FanState e FanCommand para refletir novo estado */
    updateFanStateNode();

    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &on, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(opcuaServer, NODEID_FAN_COMMAND, value);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode writeTempMax(UA_Server *server,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *nodeId, void *nodeContext,
                                  const UA_NumericRange *range, const UA_DataValue *data)
{
    (void)server;
    (void)sessionId;
    (void)sessionContext;
    (void)nodeId;
    (void)nodeContext;
    (void)range;

    if (!data || !UA_Variant_isScalar(&data->value) ||
        data->value.type != &UA_TYPES[UA_TYPES_FLOAT])
    {
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    float *val = (float*)data->value.data;
    if (val == NULL)
    {
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    tempMax = *val;

    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &tempMax, &UA_TYPES[UA_TYPES_FLOAT]);
    UA_Server_writeValue(opcuaServer, NODEID_TEMP_MAX, value);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode writeTempMin(UA_Server *server,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *nodeId, void *nodeContext,
                                  const UA_NumericRange *range, const UA_DataValue *data)
{
    (void)server;
    (void)sessionId;
    (void)sessionContext;
    (void)nodeId;
    (void)nodeContext;
    (void)range;

    if (!data || !UA_Variant_isScalar(&data->value) ||
        data->value.type != &UA_TYPES[UA_TYPES_FLOAT])
    {
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    float *val = (float*)data->value.data;
    if (val == NULL)
    {
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    tempMin = *val;

    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &tempMin, &UA_TYPES[UA_TYPES_FLOAT]);
    UA_Server_writeValue(opcuaServer, NODEID_TEMP_MIN, value);

    return UA_STATUSCODE_GOOD;
}
