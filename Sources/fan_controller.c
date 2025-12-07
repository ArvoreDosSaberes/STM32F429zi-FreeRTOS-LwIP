/**
 * @file fan_controller.c
 * @brief Implementação do controlador de ventilador via GPIO.
 * 
 * Controla um ventilador através de um pino GPIO do STM32F429ZI.
 * Suporta comandos via string (MQTT) e controle direto.
 */

#include "fan_controller.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*-----------------------------------------------------------------------------
 * Variáveis Privadas
 *----------------------------------------------------------------------------*/

/** @brief Estado atual do ventilador. */
static FanState currentState = FAN_STATE_OFF;

/** @brief Flag indicando se o controlador foi inicializado. */
static bool controllerInitialized = false;

/*-----------------------------------------------------------------------------
 * Funções Privadas
 *----------------------------------------------------------------------------*/

/**
 * @brief Converte string para minúsculas (in-place limitado).
 * 
 * @param str String a converter.
 * @param len Tamanho máximo a processar.
 * @param out Buffer de saída.
 * @param outLen Tamanho do buffer de saída.
 */
static void strToLower(const char *str, uint16_t len, char *out, uint16_t outLen)
{
    uint16_t i;
    uint16_t maxLen = (len < outLen - 1) ? len : (outLen - 1);
    
    for (i = 0; i < maxLen && str[i] != '\0'; i++)
    {
        out[i] = (char)tolower((unsigned char)str[i]);
    }
    out[i] = '\0';
}

/**
 * @brief Habilita o clock do GPIO do ventilador.
 */
static void enableGpioClock(void)
{
    /* Habilitar clock do GPIOB (onde está o pino do ventilador) */
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

/*-----------------------------------------------------------------------------
 * Implementação das Funções Públicas
 *----------------------------------------------------------------------------*/

int fanControllerInit(void)
{
    GPIO_InitTypeDef gpioInit = {0};
    
    printf("[FanCtrl] Inicializando controlador do ventilador...\r\n");
    
    /* Habilitar clock do GPIO */
    enableGpioClock();
    
    /* Configurar pino como saída push-pull */
    gpioInit.Pin = FAN_GPIO_PIN;
    gpioInit.Mode = GPIO_MODE_OUTPUT_PP;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_LOW;
    
    HAL_GPIO_Init(FAN_GPIO_PORT, &gpioInit);
    
    /* Iniciar com ventilador desligado */
    fanControllerOff();
    
    controllerInitialized = true;
    printf("[FanCtrl] Inicializado. Pino: PB0, Estado inicial: OFF\r\n");
    
    return 0;
}

void fanControllerOn(void)
{
    if (!controllerInitialized)
    {
        printf("[FanCtrl] Erro: Controlador nao inicializado\r\n");
        return;
    }
    
#if FAN_ACTIVE_HIGH
    HAL_GPIO_WritePin(FAN_GPIO_PORT, FAN_GPIO_PIN, GPIO_PIN_SET);
#else
    HAL_GPIO_WritePin(FAN_GPIO_PORT, FAN_GPIO_PIN, GPIO_PIN_RESET);
#endif
    
    currentState = FAN_STATE_ON;
    printf("[FanCtrl] Ventilador LIGADO\r\n");
}

void fanControllerOff(void)
{
    if (!controllerInitialized)
    {
        printf("[FanCtrl] Erro: Controlador nao inicializado\r\n");
        return;
    }
    
#if FAN_ACTIVE_HIGH
    HAL_GPIO_WritePin(FAN_GPIO_PORT, FAN_GPIO_PIN, GPIO_PIN_RESET);
#else
    HAL_GPIO_WritePin(FAN_GPIO_PORT, FAN_GPIO_PIN, GPIO_PIN_SET);
#endif
    
    currentState = FAN_STATE_OFF;
    printf("[FanCtrl] Ventilador DESLIGADO\r\n");
}

void fanControllerToggle(void)
{
    if (currentState == FAN_STATE_ON)
    {
        fanControllerOff();
    }
    else
    {
        fanControllerOn();
    }
}

void fanControllerSetState(FanState state)
{
    if (state == FAN_STATE_ON)
    {
        fanControllerOn();
    }
    else
    {
        fanControllerOff();
    }
}

FanState fanControllerGetState(void)
{
    return currentState;
}

bool fanControllerIsOn(void)
{
    return (currentState == FAN_STATE_ON);
}

FanCommand fanControllerParseCommand(const char *cmdStr, uint16_t cmdLen)
{
    char lowerCmd[32];
    
    if (cmdStr == NULL || cmdLen == 0)
    {
        return FAN_CMD_INVALID;
    }
    
    /* Converter para minúsculas para comparação case-insensitive */
    strToLower(cmdStr, cmdLen, lowerCmd, sizeof(lowerCmd));
    
    /* Comandos para ligar */
    if (strcmp(lowerCmd, "on") == 0 ||
        strcmp(lowerCmd, "1") == 0 ||
        strcmp(lowerCmd, "true") == 0 ||
        strcmp(lowerCmd, "liga") == 0 ||
        strcmp(lowerCmd, "ligar") == 0)
    {
        return FAN_CMD_ON;
    }
    
    /* Comandos para desligar */
    if (strcmp(lowerCmd, "off") == 0 ||
        strcmp(lowerCmd, "0") == 0 ||
        strcmp(lowerCmd, "false") == 0 ||
        strcmp(lowerCmd, "desliga") == 0 ||
        strcmp(lowerCmd, "desligar") == 0)
    {
        return FAN_CMD_OFF;
    }
    
    /* Comandos para inverter */
    if (strcmp(lowerCmd, "toggle") == 0 ||
        strcmp(lowerCmd, "2") == 0 ||
        strcmp(lowerCmd, "inverte") == 0 ||
        strcmp(lowerCmd, "inverter") == 0)
    {
        return FAN_CMD_TOGGLE;
    }
    
    return FAN_CMD_INVALID;
}

FanState fanControllerExecuteCommand(FanCommand cmd)
{
    switch (cmd)
    {
        case FAN_CMD_ON:
            fanControllerOn();
            break;
            
        case FAN_CMD_OFF:
            fanControllerOff();
            break;
            
        case FAN_CMD_TOGGLE:
            fanControllerToggle();
            break;
            
        case FAN_CMD_INVALID:
        default:
            printf("[FanCtrl] Comando invalido ignorado\r\n");
            break;
    }
    
    return currentState;
}

const char* fanControllerGetStateString(void)
{
    return (currentState == FAN_STATE_ON) ? "ON" : "OFF";
}
