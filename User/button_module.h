/**
  ******************************************************************************
  * @file    button_module.h
  * @author  
  * @version 
  * @date    
  * @brief   
  ******************************************************************************
  * @attention

  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __Button_Module_H
#define __Button_Module_H

/* Includes ------------------------------------------------------------------*/

#include "main.h"

/* Private typedef -----------------------------------------------------------*/


typedef enum 
{
	BUTTON12 = 0,

} Button_TypeDef;



typedef struct
{
	u32 DebounceCounter;
	u32 RetentionCounter;
	u8 Tigger;
} BUTTON_t;



/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void BUTTON_Init (void);
void BUTTON_Processing(void);

void BUTTON12_Click_Callback(void);
void BUTTON12_Retention_Callback(void);

#endif /*__Button_Module_H */



/******************END OF FILE*************************************************/
