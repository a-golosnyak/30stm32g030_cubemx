/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : module_name.c
  * @brief          : module_name driver / logic implementation
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes --------------------------------------------------------------------*/
#include <_module_template.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
MODULE_t MODULE;
/* USER CODE END PV */

/* Private function prototypes ------------------------------------------------*/
/* USER CODE BEGIN PFP */
void MODULE_Init(void);
void MODULE_Processing(void);
/* USER CODE END PFP */

/* Private user code -----------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* Exported functions ----------------------------------------------------------*/
/* USER CODE BEGIN EF */

void MODULE_Processing(void)
{
	switch(MODULE.mainStateMachine) {
	case 0:
		MODULE.mainStateMachine++;
		break;

	case 1:
		break;

	case 2:
		break;

	default:
		MODULE.mainStateMachine = 0;
		break;
	}
}

/* USER CODE END EF */
