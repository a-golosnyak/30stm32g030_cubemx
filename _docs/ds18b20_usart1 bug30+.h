/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ds18b20.h
  * @brief          : Driver for DS18B20 (1-Wire over USART/DMA trick, multi-drop)
  ******************************************************************************
  * @note
  *  Physical:      DQ -> PB6 (USART1_TX, AF0), Open-Drain, external pull-up 4.7k -> 3.3V
  *                 (TSSOP20 package pin #20 is bonded to PB3/PB4/PB5/PB6 —
  *                  select PB6 specifically in CubeMX Pinout view to get USART1_TX here)
  *  CubeMX config required:
  *    - USART1: Mode = Single Wire (Half-Duplex), Asynchronous
  *    - USART1 DMA: add DMA request for both USART1_RX and USART1_TX,
  *                  Mode = Normal (not Circular), enable NVIC global interrupt
  *    - PB6: Alternate Function Open Drain, no internal pull (external 4.7k used)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DS18B20_H
#define __DS18B20_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef struct
{
    uint8_t rom[8];        /* 64-bit ROM code (family + serial + CRC)          */
    int16_t tempRaw;       /* raw scratchpad value, fixed-point Q4: 1 LSB = 1/16 degC */
    uint8_t valid;         /* 1 = tempRaw holds a CRC-checked value            */
} DS18B20_Sensor;

typedef enum
{
    DS18B20_STATE_IDLE = 0,   /* nothing in progress, ready for StartConversion */
    DS18B20_STATE_CONVERTING, /* Convert T issued, waiting non-blocking         */
    DS18B20_STATE_READING     /* reading scratchpads (short, synchronous burst) */
} DS18B20_State;

typedef struct {
	u8	MainStateMachine;
	u32	Counter;
} DS18B20_t;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define DS18B20_MAX_SENSORS     8      /* size of internal sensor table        */
#define DS18B20_CONV_TIME_MS    850U   /* 12-bit resolution conversion time    */
#define DS18B20_INVALID_RAW     INT16_MIN  /* sentinel: no valid reading yet   */
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */
void            DS18B20_Init(UART_HandleTypeDef *huart);
uint8_t         DS18B20_SearchAll(void);
void            DS18B20_StartConversion(void);
void            DS18B20_Process(void);
uint8_t         DS18B20_IsDataReady(void);
void            DS18B20_ClearDataReady(void);
DS18B20_State   DS18B20_GetState(void);
uint8_t         DS18B20_GetSensorCount(void);
int16_t         DS18B20_GetTemperatureRaw(uint8_t index);    /* Q4: 1 LSB = 1/16 degC, or DS18B20_INVALID_RAW */
int32_t         DS18B20_GetTemperatureCentiC(uint8_t index); /* hundredths of degC, e.g. 2344 = 23.44 C, or INT32_MIN */
int32_t         DS18B20_RawToCentiC(int16_t raw);             /* integer-only Q4 -> centi-degrees conversion */
const uint8_t  *DS18B20_GetROM(uint8_t index);
uint8_t         DS18B20_QuickTestSkipROM(int16_t *outRaw);   /* DIAGNOSTIC ONLY */
void 			DS18B20_Processing(int16_t *outRaw);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

#ifdef __cplusplus
}
#endif

#endif /* __DS18B20_H */
