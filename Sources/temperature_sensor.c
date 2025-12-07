/**
 * @file temperature_sensor.c
 * @brief Implementação do sensor de temperatura interno do STM32F429ZI.
 * 
 * Utiliza o ADC1 com o canal interno do sensor de temperatura (IN18)
 * e aplica os dados de calibração de fábrica para leitura precisa.
 */

#include "temperature_sensor.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

/*-----------------------------------------------------------------------------
 * Variáveis Privadas
 *----------------------------------------------------------------------------*/

/** @brief Handle do ADC para leitura de temperatura. */
static ADC_HandleTypeDef hAdc;

/** @brief Flag indicando se o sensor foi inicializado. */
static bool sensorInitialized = false;

/** @brief Valores de calibração de fábrica. */
static uint16_t tsCalibration1 = 0;
static uint16_t tsCalibration2 = 0;

/*-----------------------------------------------------------------------------
 * Funções Privadas
 *----------------------------------------------------------------------------*/

/**
 * @brief Configura o clock do ADC1.
 */
static void adcClockConfig(void)
{
    /* Habilitar clock do ADC1 */
    __HAL_RCC_ADC1_CLK_ENABLE();
}

/**
 * @brief Converte valor ADC bruto para temperatura em Celsius.
 * 
 * Utiliza os valores de calibração de fábrica para conversão precisa.
 * 
 * Fórmula (RM0090):
 * Temperature = ((TS_CAL2_TEMP - TS_CAL1_TEMP) / (TS_CAL2 - TS_CAL1)) 
 *               * (TS_DATA - TS_CAL1) + TS_CAL1_TEMP
 * 
 * @param adcValue Valor bruto do ADC (0-4095).
 * @return Temperatura em graus Celsius.
 */
static float convertAdcToTemperature(uint16_t adcValue)
{
    float temperature;
    
    /* Usar calibração de fábrica se disponível */
    if ((tsCalibration1 != 0) && (tsCalibration2 != 0) && 
        (tsCalibration2 != tsCalibration1))
    {
        /* Fórmula com calibração de fábrica */
        temperature = (TEMP_SENSOR_CAL2_TEMP - TEMP_SENSOR_CAL1_TEMP);
        temperature /= (float)(tsCalibration2 - tsCalibration1);
        temperature *= (float)(adcValue - tsCalibration1);
        temperature += TEMP_SENSOR_CAL1_TEMP;
    }
    else
    {
        /* Fallback: usar valores típicos do datasheet */
        float voltage = ((float)adcValue * TEMP_SENSOR_VREF_MV) / 
                        (float)TEMP_SENSOR_ADC_MAX_VALUE;
        temperature = ((voltage - TEMP_SENSOR_V25_MV) / TEMP_SENSOR_AVG_SLOPE_MV) + 25.0f;
    }
    
    return temperature;
}

/*-----------------------------------------------------------------------------
 * Implementação das Funções Públicas
 *----------------------------------------------------------------------------*/

TemperatureSensorError temperatureSensorInit(void)
{
    ADC_ChannelConfTypeDef channelConfig = {0};
    
    /* Ler valores de calibração de fábrica */
    tsCalibration1 = *TEMP_SENSOR_CAL1_ADDR;
    tsCalibration2 = *TEMP_SENSOR_CAL2_ADDR;
    
    printf("[TempSensor] Calibracao: CAL1=%u (30C), CAL2=%u (110C)\r\n",
           tsCalibration1, tsCalibration2);
    
    /* Configurar clock do ADC */
    adcClockConfig();
    
    /* Configuração do ADC1 */
    hAdc.Instance = ADC1;
    hAdc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;    /* APB2/4 = 22.5MHz */
    hAdc.Init.Resolution = ADC_RESOLUTION_12B;
    hAdc.Init.ScanConvMode = DISABLE;
    hAdc.Init.ContinuousConvMode = DISABLE;
    hAdc.Init.DiscontinuousConvMode = DISABLE;
    hAdc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hAdc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hAdc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hAdc.Init.NbrOfConversion = 1;
    hAdc.Init.DMAContinuousRequests = DISABLE;
    hAdc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    
    if (HAL_ADC_Init(&hAdc) != HAL_OK)
    {
        printf("[TempSensor] Erro: Falha ao inicializar ADC\r\n");
        return TEMP_SENSOR_ERROR_INIT;
    }
    
    /* Configurar canal do sensor de temperatura (IN18) */
    channelConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    channelConfig.Rank = 1;
    /* Tempo de amostragem: 480 ciclos (máximo) para maior estabilidade */
    channelConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    
    if (HAL_ADC_ConfigChannel(&hAdc, &channelConfig) != HAL_OK)
    {
        printf("[TempSensor] Erro: Falha ao configurar canal\r\n");
        return TEMP_SENSOR_ERROR_INIT;
    }
    
    sensorInitialized = true;
    printf("[TempSensor] Inicializado com sucesso\r\n");
    
    return TEMP_SENSOR_OK;
}

TemperatureSensorError temperatureSensorRead(float *temperature)
{
    uint16_t rawValue;
    TemperatureSensorError result;
    
    if (temperature == NULL)
    {
        return TEMP_SENSOR_ERROR_READ;
    }
    
    /* Ler valor bruto do ADC */
    result = temperatureSensorReadRaw(&rawValue);
    if (result != TEMP_SENSOR_OK)
    {
        return result;
    }
    
    /* Converter para temperatura */
    *temperature = convertAdcToTemperature(rawValue);
    
    return TEMP_SENSOR_OK;
}

TemperatureSensorError temperatureSensorReadRaw(uint16_t *rawValue)
{
    if (!sensorInitialized)
    {
        return TEMP_SENSOR_ERROR_NOT_INIT;
    }
    
    if (rawValue == NULL)
    {
        return TEMP_SENSOR_ERROR_READ;
    }
    
    /* Iniciar conversão */
    if (HAL_ADC_Start(&hAdc) != HAL_OK)
    {
        return TEMP_SENSOR_ERROR_READ;
    }
    
    /* Aguardar conversão (timeout: 100ms) */
    if (HAL_ADC_PollForConversion(&hAdc, 100) != HAL_OK)
    {
        HAL_ADC_Stop(&hAdc);
        return TEMP_SENSOR_ERROR_TIMEOUT;
    }
    
    /* Ler valor convertido */
    *rawValue = (uint16_t)HAL_ADC_GetValue(&hAdc);
    
    /* Parar ADC */
    HAL_ADC_Stop(&hAdc);
    
    return TEMP_SENSOR_OK;
}

void temperatureSensorGetCalibration(uint16_t *cal1, uint16_t *cal2)
{
    if (cal1 != NULL)
    {
        *cal1 = tsCalibration1;
    }
    if (cal2 != NULL)
    {
        *cal2 = tsCalibration2;
    }
}

bool temperatureSensorIsInitialized(void)
{
    return sensorInitialized;
}

void temperatureSensorDeinit(void)
{
    if (sensorInitialized)
    {
        HAL_ADC_DeInit(&hAdc);
        __HAL_RCC_ADC1_CLK_DISABLE();
        sensorInitialized = false;
        printf("[TempSensor] Desligado\r\n");
    }
}
