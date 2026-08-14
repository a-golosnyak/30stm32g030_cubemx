/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : module_name.h
  * @brief          : Header for module_name.c file.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MODULE_NAME_H
#define __MODULE_NAME_H

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

typedef enum
{
	VREF = 0,
	TEMP,
	VNTC,
	VSHUNHT,
	VOPAMP,
	VBAT,
	CODSIZE
}AdcCod_e;

typedef struct
{
	u16 AdcCod[CODSIZE];
	Integrator_t	IntegrVREF;
	Integrator_t	IntegrTEMP;
	Integrator_t	IntegrVNTC;
	Integrator_t	IntegrVSHUNT;
	Integrator_t	IntegrVOPAMP;
	Integrator_t	IntegrVBAT;

	s16 Vref;
	s16 VrefOld;
	s16 Temp;
	s16 TempOld;
	s16 Vntc;
	s16 VntcOld;
	s16 Vshunt;
	s16 VshuntOld;
	s16 VopAmp;
	s16 VopAmpOld;
	s16 Vbat;
	s16 VbatOld;

	s16 Vdda;
	s16 VddaOld;
	s16 Vmcu;
}PWRMNG_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

extern PWRMNG_t PWRMNG;

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */

void POWER_Init(void);
void PWRMNG_Processing(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

#ifdef __cplusplus
}
#endif

#endif /* __MODULE_NAME_H */
