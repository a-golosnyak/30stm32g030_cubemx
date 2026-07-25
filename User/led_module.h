/**
  ******************************************************************************
  * @file    Led.h
  * @author  
  * @version 
  * @date    
  * @brief   
  ******************************************************************************
  * @attention

  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LED_Module_H
#define __LED_Module_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private define ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/

typedef enum 
{
	LED0 = 0,
	LED1 = 1
} Led_TypeDef;

typedef struct
{
	u32 Counter;
	u16 TimeOff;
	u16 TimeOn;
	u16 Times;
} LEDs_t;


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

void LED_Init (void);
void LED_Processing(void);
void LED_TurnOn(Led_TypeDef led, u32 time_ms);
void LED_TurnOnBlink(Led_TypeDef led, u16 time_on, u16 time_off, u16 times);

void LED_On(Led_TypeDef Led);
void LED_Off(Led_TypeDef Led);
void LED_Toggle(Led_TypeDef Led);


#endif /*__LED_Module_H */



/******************END OF FILE*************************************************/
