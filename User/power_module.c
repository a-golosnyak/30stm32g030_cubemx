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
	/* DMA must be disabled during calibration */
	LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_NONE);

	if (LL_ADC_IsEnabled(ADC1) == 0)
	{
		LL_ADC_StartCalibration(ADC1);

		uint32_t timeout = SystemCounter + 1000;
		while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0)
		{
			if (timeout < SystemCounter)
			{
				Error_Handler();
			}
		}

		LL_ADC_Enable(ADC1);

		timeout = SystemCounter + 1000;
		while (LL_ADC_IsActiveFlag_ADRDY(ADC1) == 0)
		{
			if (timeout < SystemCounter)
			{
				Error_Handler();
			}
		}

		LL_ADC_ClearFlag_ADRDY(ADC1);
	}


	LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);
	LL_DMA_ClearFlag_TC5(DMA1);
	LL_DMA_ClearFlag_TE5(DMA1);


	LL_DMA_ConfigAddresses(
		DMA1, LL_DMA_CHANNEL_5,
		LL_ADC_DMA_GetRegAddr(ADC1, LL_ADC_DMA_REG_REGULAR_DATA),
		(uint32_t)PWRMNG.AdcCod,
		LL_DMA_DIRECTION_PERIPH_TO_MEMORY
	);

	LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
	LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, 6);
	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);

	LL_ADC_REG_StartConversion(ADC1);
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
	PWRMNG.IntegrVBAT.Sum += KalmanFilter6(PWRMNG.AdcCod[VBAT]);

	if(PWRMNG.IntegrVREF.Index == 9)
	{
//		LED_Off(LED0);
		//----- VRef -----------------------------------------
		uint32_t vRefInt_raw = PWRMNG.IntegrVREF.Sum / 16;							// 10-integrator, 16-oversmpling
		PWRMNG.Vref = (vRefInt_raw * 330u) / (4095u);

		//----- Vdda -----------------------------------------
		uint32_t vrefint_cal = *VREFINT_CAL_ADDR;
		PWRMNG.Vdda = (VREFINT_CAL_VREF * vrefint_cal * 10)/vRefInt_raw;

		//----- Vtemp ----------------------------------------
		int32_t tempSensCal1Addr = *TEMPSENSOR_CAL1_ADDR;							// 1030
		int32_t tempSensCal2Addr = *TEMPSENSOR_CAL2_ADDR;							// 1367

		PWRMNG.IntegrTEMP.Sum = PWRMNG.IntegrTEMP.Sum / 1/16;							// На 10 не делим чтобы сохранить десятые 26,5
		int32_t vTempCorrected = (PWRMNG.IntegrTEMP.Sum * PWRMNG.Vdda / 3000);		// 14000
		PWRMNG.Temp = ((100*(vTempCorrected-tempSensCal1Addr*10))/(tempSensCal2Addr - tempSensCal1Addr))+300;

		//----- Vntc -----------------------------------------
		PWRMNG.IntegrVNTC.Sum = PWRMNG.IntegrVNTC.Sum/10/16;
		PWRMNG.Vntc = NTC_GetTemperature(PWRMNG.IntegrVNTC.Sum);
		//----- Vbat -----------------------------------------

//		PWRMNG.VshuntCod = PWRMNG.IntegrVSHUNT.Sum / 160;
		PWRMNG.Vbat = PWRMNG.AdcCod[VBAT] / 16;
		PWRMNG.Vshunt = (PWRMNG.IntegrVSHUNT.Sum * PWRMNG.Vdda) / (4095u*160);		// 10-integrator, 16-oversmpling
		PWRMNG.VopAmp = (PWRMNG.IntegrVOPAMP.Sum * PWRMNG.Vdda) / (4095u*160);		// 10-integrator, 16-oversmpling
//		PWRMNG.Vbat = (PWRMNG.IntegrVSHUNT.Sum * PWRMNG.Vdda) / (4095u*160);		// 10-integrator, 16-oversmpling
//		PWRMNG.Vbat = (PWRMNG.IntegrVBAT.Sum * 330u) / (4095u*16*2);				// 10-integrator, 16-oversmpling
//		PWRMNG.Vntc = (PWRMNG.IntegrVNTC.Sum * 330)/(4095*16);

		//----------------------------------------------------

//		if(PWRMNG.IntegrVNTC.Sum < 0)
//			PWRMNG.IntegrVNTC.Sum = 0;
//		else if (PWRMNG.IntegrVNTC.Sum > 588)
//			PWRMNG.IntegrVNTC.Sum = 588;
		PWRMNG.Vbat = PWRMNG.Vbat - 7;

		if(PWRMNG.Vbat < 0)
			PWRMNG.Vbat = 0;
//		else if (PWRMNG.VshuntCod > 588)
//			PWRMNG.VshuntCod = 588;

//		LL_TIM_SetAutoReload(TIM1, PWRMNG.VshuntCod);
		LL_TIM_OC_SetCompareCH1(TIM1, PWRMNG.Vbat);
		//----------------------------------------------------

		PWRMNG.IntegrVREF.Index = 0;
		PWRMNG.IntegrVREF.Sum = 0;
		PWRMNG.IntegrTEMP.Sum = 0;
		PWRMNG.IntegrVNTC.Sum = 0;
		PWRMNG.IntegrVSHUNT.Sum = 0;
		PWRMNG.IntegrVOPAMP.Sum = 0;
		PWRMNG.IntegrVBAT.Sum = 0;
//		LED_On(LED0);
	}
	else
		PWRMNG.IntegrVREF.Index++;


//	LED_Off(LED1);
	__NOP();
}

/* USER CODE END 0 */


















