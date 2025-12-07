/**
 * @file fan_controller.h
 * @brief Interface para controle do ventilador via GPIO no STM32F429ZI.
 * 
 * Este módulo controla um ventilador através de um pino GPIO.
 * Suporta controle ON/OFF simples e pode ser expandido para PWM.
 * 
 * @note O pino padrão é PB0, mas pode ser alterado via definições.
 */

#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*-----------------------------------------------------------------------------
 * Configuração do Hardware
 *----------------------------------------------------------------------------*/

/**
 * @brief Porta GPIO para controle do ventilador.
 *        Padrão: GPIOB
 */
#ifndef FAN_GPIO_PORT
    #define FAN_GPIO_PORT           GPIOB
#endif

/**
 * @brief Pino GPIO para controle do ventilador.
 *        Padrão: GPIO_PIN_0 (PB0)
 */
#ifndef FAN_GPIO_PIN
    #define FAN_GPIO_PIN            GPIO_PIN_0
#endif

/**
 * @brief Nível lógico para ligar o ventilador.
 *        1 = Ativo alto (lógica positiva)
 *        0 = Ativo baixo (lógica negativa/relé)
 */
#ifndef FAN_ACTIVE_HIGH
    #define FAN_ACTIVE_HIGH         1
#endif

/*-----------------------------------------------------------------------------
 * Estados do Ventilador
 *----------------------------------------------------------------------------*/

/**
 * @brief Estados possíveis do ventilador.
 */
typedef enum {
    FAN_STATE_OFF       = 0,    /**< Ventilador desligado */
    FAN_STATE_ON        = 1,    /**< Ventilador ligado */
    FAN_STATE_ERROR     = -1    /**< Erro no controle */
} FanState;

/**
 * @brief Comandos de controle do ventilador.
 *        Usados para parsing de mensagens MQTT.
 */
typedef enum {
    FAN_CMD_OFF         = 0,    /**< Comando para desligar */
    FAN_CMD_ON          = 1,    /**< Comando para ligar */
    FAN_CMD_TOGGLE      = 2,    /**< Comando para inverter estado */
    FAN_CMD_INVALID     = -1    /**< Comando inválido */
} FanCommand;

/*-----------------------------------------------------------------------------
 * Funções Públicas
 *----------------------------------------------------------------------------*/

/**
 * @brief Inicializa o controlador do ventilador.
 * 
 * Configura o pino GPIO como saída push-pull e inicializa
 * o ventilador no estado desligado.
 * 
 * @return 0 em caso de sucesso, -1 em caso de falha.
 */
int fanControllerInit(void);

/**
 * @brief Liga o ventilador.
 * 
 * Ativa o pino GPIO conforme a configuração de nível ativo.
 */
void fanControllerOn(void);

/**
 * @brief Desliga o ventilador.
 * 
 * Desativa o pino GPIO conforme a configuração de nível ativo.
 */
void fanControllerOff(void);

/**
 * @brief Inverte o estado atual do ventilador.
 * 
 * Se ligado, desliga. Se desligado, liga.
 */
void fanControllerToggle(void);

/**
 * @brief Define o estado do ventilador.
 * 
 * @param state Estado desejado (FAN_STATE_OFF ou FAN_STATE_ON).
 */
void fanControllerSetState(FanState state);

/**
 * @brief Obtém o estado atual do ventilador.
 * 
 * @return Estado atual (FAN_STATE_OFF ou FAN_STATE_ON).
 */
FanState fanControllerGetState(void);

/**
 * @brief Verifica se o ventilador está ligado.
 * 
 * @return true se ligado, false se desligado.
 */
bool fanControllerIsOn(void);

/**
 * @brief Converte uma string de comando para FanCommand.
 * 
 * Aceita os seguintes comandos (case-insensitive):
 * - "on", "1", "true", "liga" -> FAN_CMD_ON
 * - "off", "0", "false", "desliga" -> FAN_CMD_OFF
 * - "toggle", "2", "inverte" -> FAN_CMD_TOGGLE
 * 
 * @param cmdStr String do comando recebido.
 * @param cmdLen Tamanho da string do comando.
 * 
 * @return Comando interpretado ou FAN_CMD_INVALID se não reconhecido.
 */
FanCommand fanControllerParseCommand(const char *cmdStr, uint16_t cmdLen);

/**
 * @brief Executa um comando no ventilador.
 * 
 * @param cmd Comando a executar.
 * 
 * @return Novo estado do ventilador após executar o comando.
 */
FanState fanControllerExecuteCommand(FanCommand cmd);

/**
 * @brief Obtém string representando o estado do ventilador.
 * 
 * @return "ON" ou "OFF" conforme estado atual.
 */
const char* fanControllerGetStateString(void);

#ifdef __cplusplus
}
#endif

#endif /* FAN_CONTROLLER_H */
