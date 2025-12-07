/**
 * @file temperature_sensor.h
 * @brief Interface para leitura do sensor de temperatura interno do STM32F429ZI.
 * 
 * O STM32F429 possui um sensor de temperatura integrado conectado ao canal ADC
 * interno (ADC1_IN18). Este módulo configura o ADC e fornece funções para
 * leitura da temperatura do chip em graus Celsius.
 * 
 * @note A precisão típica é de ±1.5°C. Calibração adicional pode melhorar a precisão.
 * 
 * Referência: RM0090, Seção 13.10 (Temperature sensor)
 */

#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*-----------------------------------------------------------------------------
 * Definições e Constantes
 *----------------------------------------------------------------------------*/

/**
 * @brief Tensão de referência do ADC em milivolts.
 *        Valor típico para STM32F4 alimentado a 3.3V.
 */
#define TEMP_SENSOR_VREF_MV         3300

/**
 * @brief Resolução do ADC em bits.
 *        STM32F4 suporta 12, 10, 8 ou 6 bits.
 */
#define TEMP_SENSOR_ADC_RESOLUTION  12

/**
 * @brief Valor máximo do ADC para a resolução configurada.
 */
#define TEMP_SENSOR_ADC_MAX_VALUE   ((1 << TEMP_SENSOR_ADC_RESOLUTION) - 1)

/**
 * @brief Valores de calibração do sensor de temperatura (datasheet STM32F429).
 *        V25: Tensão a 25°C em mV (típico 760mV)
 *        Avg_Slope: Inclinação média em mV/°C (típico 2.5mV/°C)
 */
#define TEMP_SENSOR_V25_MV          760
#define TEMP_SENSOR_AVG_SLOPE_MV    2.5f

/**
 * @brief Endereços de calibração na memória do STM32F429.
 *        TS_CAL1: Valor ADC a 30°C (VDDA = 3.3V)
 *        TS_CAL2: Valor ADC a 110°C (VDDA = 3.3V)
 */
#define TEMP_SENSOR_CAL1_ADDR       ((uint16_t*)0x1FFF7A2C)
#define TEMP_SENSOR_CAL2_ADDR       ((uint16_t*)0x1FFF7A2E)
#define TEMP_SENSOR_CAL1_TEMP       30.0f
#define TEMP_SENSOR_CAL2_TEMP       110.0f

/*-----------------------------------------------------------------------------
 * Códigos de Erro
 *----------------------------------------------------------------------------*/

/**
 * @brief Códigos de retorno para operações do sensor de temperatura.
 */
typedef enum {
    TEMP_SENSOR_OK              = 0,    /**< Operação bem-sucedida */
    TEMP_SENSOR_ERROR_INIT      = -1,   /**< Falha na inicialização */
    TEMP_SENSOR_ERROR_READ      = -2,   /**< Falha na leitura */
    TEMP_SENSOR_ERROR_TIMEOUT   = -3,   /**< Timeout na conversão */
    TEMP_SENSOR_ERROR_NOT_INIT  = -4    /**< Sensor não inicializado */
} TemperatureSensorError;

/*-----------------------------------------------------------------------------
 * Funções Públicas
 *----------------------------------------------------------------------------*/

/**
 * @brief Inicializa o ADC para leitura do sensor de temperatura interno.
 * 
 * Configura o ADC1 com o canal do sensor de temperatura (IN18),
 * habilita o sensor e configura os parâmetros de amostragem.
 * 
 * @return TEMP_SENSOR_OK em caso de sucesso, código de erro caso contrário.
 * 
 * @note Esta função deve ser chamada antes de qualquer leitura.
 * @note O sensor de temperatura requer um tempo de amostragem mínimo de 10µs.
 */
TemperatureSensorError temperatureSensorInit(void);

/**
 * @brief Lê a temperatura atual do chip em graus Celsius.
 * 
 * Realiza uma conversão ADC do sensor de temperatura interno e
 * converte o valor para temperatura usando os dados de calibração.
 * 
 * @param[out] temperature Ponteiro para armazenar a temperatura lida (°C).
 * 
 * @return TEMP_SENSOR_OK em caso de sucesso, código de erro caso contrário.
 * 
 * @note A leitura é bloqueante e aguarda a conversão completar.
 * @note Tempo típico de leitura: < 1ms.
 * 
 * @example
 * @code
 * float temp;
 * if (temperatureSensorRead(&temp) == TEMP_SENSOR_OK) {
 *     printf("Temperatura: %.1f C\n", temp);
 * }
 * @endcode
 */
TemperatureSensorError temperatureSensorRead(float *temperature);

/**
 * @brief Lê o valor bruto (raw) do ADC do sensor de temperatura.
 * 
 * Útil para debug ou quando se deseja aplicar calibração personalizada.
 * 
 * @param[out] rawValue Ponteiro para armazenar o valor ADC (0-4095 para 12 bits).
 * 
 * @return TEMP_SENSOR_OK em caso de sucesso, código de erro caso contrário.
 */
TemperatureSensorError temperatureSensorReadRaw(uint16_t *rawValue);

/**
 * @brief Obtém os valores de calibração de fábrica.
 * 
 * Retorna os valores de calibração gravados na memória do chip durante
 * a fabricação. Úteis para calibração precisa.
 * 
 * @param[out] cal1 Valor ADC a 30°C (pode ser NULL se não necessário).
 * @param[out] cal2 Valor ADC a 110°C (pode ser NULL se não necessário).
 */
void temperatureSensorGetCalibration(uint16_t *cal1, uint16_t *cal2);

/**
 * @brief Verifica se o sensor de temperatura está inicializado.
 * 
 * @return true se inicializado, false caso contrário.
 */
bool temperatureSensorIsInitialized(void);

/**
 * @brief Desinicializa o sensor de temperatura.
 * 
 * Desliga o ADC e libera recursos. A inicialização é necessária
 * novamente antes de novas leituras.
 */
void temperatureSensorDeinit(void);

#ifdef __cplusplus
}
#endif

#endif /* TEMPERATURE_SENSOR_H */
