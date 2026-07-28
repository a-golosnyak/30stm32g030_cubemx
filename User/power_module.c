/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : module_name.c
  * @brief          : module_name driver / logic implementation
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes --------------------------------------------------------------------*/


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "power_module.h"
#include "led_module.h"
#include "string.h"
#include "ntc_termistor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
PWRMNG_t PWRMNG;
/* USER CODE END PV */

/* Private function prototypes ------------------------------------------------*/
/* USER CODE BEGIN PFP */
void ADC_LowLevel_Init(void);
/* USER CODE END PFP */

/* Private user code -----------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*********************************************************************************
  * @brief
  * @param  None
  * @retval None
*********************************************************************************/
void ADC_LowLevel_Init(void)
{
  if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
  {
	  Error_Handler();
  }

  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)PWRMNG.AdcCod, 5);
}

/*********************************************************************************
  * @brief
  * @param  None
  * @retval None
*********************************************************************************/
void POWER_Init(void)
{
	memset(&PWRMNG, 0, sizeof(PWRMNG_t) );
	ADC_LowLevel_Init();
}

/*********************************************************************************
  * @brief
  * @param  None
  * @retval None
*********************************************************************************/
void PWRMNG_Processing(void)
{
//	LED_On(LED1);

	PWRMNG.IntegrVREF.Sum += KalmanFilter1(PWRMNG.AdcCod[VREF]);
	PWRMNG.IntegrTEMP.Sum += KalmanFilter2(PWRMNG.AdcCod[TEMP]);
	PWRMNG.IntegrVNTC.Sum += KalmanFilter3(PWRMNG.AdcCod[VNTC]);
	PWRMNG.IntegrVSHUNT.Sum += KalmanFilter4(PWRMNG.AdcCod[VSHUNHT]);
	PWRMNG.IntegrVOPAMP.Sum += KalmanFilter5(PWRMNG.AdcCod[VOPAMP]);
//	PWRMNG.IntegrVBAT.Sum += KalmanFilter4(PWRMNG.AdcCod[VBAT]);

	if(PWRMNG.IntegrVREF.Index == 19)
	{
		//----- VRef -----------------------------------------
		uint32_t vRefInt_raw = PWRMNG.IntegrVREF.Sum / 20u /16;						// 10-integrator, 16-oversmpling
		PWRMNG.Vref = (vRefInt_raw * 3300u) / (4095u);

		//----- Vdda -----------------------------------------
		uint32_t vrefint_cal = *VREFINT_CAL_ADDR;
		PWRMNG.Vdda = (VREFINT_CAL_VREF * vrefint_cal)/vRefInt_raw;

		//----- Vtemp ----------------------------------------
		int32_t tempSensCal1Addr = *TEMPSENSOR_CAL1_ADDR;							// 1030
		int32_t tempSensCal2Addr = *TEMPSENSOR_CAL2_ADDR;							// 1367

		PWRMNG.IntegrTEMP.Sum = PWRMNG.IntegrTEMP.Sum / 2/16;							// На 10 не делим чтобы сохранить десятые 26,5
		int32_t vTempCorrected = (PWRMNG.IntegrTEMP.Sum * PWRMNG.Vdda / 3000);		// 14000
		PWRMNG.Temp = ((100*(vTempCorrected-tempSensCal1Addr*10))/(tempSensCal2Addr - tempSensCal1Addr))+300;

		//----- Vntc -----------------------------------------
		PWRMNG.IntegrVNTC.Sum = PWRMNG.IntegrVNTC.Sum/20/16;
		PWRMNG.Vntc = NTC_GetTemperature(PWRMNG.IntegrVNTC.Sum);
		//----- Vbat -----------------------------------------

		PWRMNG.Vshunt = (PWRMNG.IntegrVSHUNT.Sum * 330u) / (4095u*16*2);			// 10-integrator, 16-oversmpling
		PWRMNG.VopAmp = (PWRMNG.IntegrVOPAMP.Sum * 330u) / (4095u*16*2);			// 10-integrator, 16-oversmpling
		PWRMNG.Vbat = (PWRMNG.IntegrVBAT.Sum * 330u) / (4095u*16*2);				// 10-integrator, 16-oversmpling
//		PWRMNG.Vntc = (PWRMNG.IntegrVNTC.Sum * 330)/(4095*16);

		PWRMNG.IntegrVREF.Index = 0;
		PWRMNG.IntegrVREF.Sum = 0;
		PWRMNG.IntegrTEMP.Sum = 0;
		PWRMNG.IntegrVNTC.Sum = 0;
		PWRMNG.IntegrVSHUNT.Sum = 0;
		PWRMNG.IntegrVOPAMP.Sum = 0;
		PWRMNG.IntegrVBAT.Sum = 0;
	}
	else
		PWRMNG.IntegrVREF.Index++;


//	LED_Off(LED1);
	__NOP();
}

/* USER CODE END 0 */


















