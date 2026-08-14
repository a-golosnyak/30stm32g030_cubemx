/**
  ******************************************************************************
  * @file    button_module.c
  * @author  
  * @version 
  * @date    
  * @brief   
  ******************************************************************************
  * @attention
  Нужно:
  1. Определить количество кнопок.
  2. Раскоментировать указатели на прототипы нужных функций. Нужно использовать 
  обе функции ( Клик и Удержание).
  3. В BUTTON_PORT и BUTTON_PIN добавитьнужную периферию.
  4. Переписать функцию инициализации.
  5. Добавить функцию в прерывание SysTick.
  6. Написать тела ф-ций обработчиков нажатий.
  ******************************************************************************
  */
	
/* Includes ------------------------------------------------------------------*/

#include "string.h"	
#include "button_module.h"
#include "led_module.h"

/* Private define ------------------------------------------------------------*/

#define BUTTONn			1

#define BOUNCE_TIME		25			// ms ?
#define RETENTION_TIME	400			// ms ? 20000 ~ 300ms
#define REPEAT_TIME		400			// ms ? Время, через которое повторять RetainAction

/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
typedef enum
{
	CLICK = 0,
	RETENTION,
	N_ACTIONS
} Button_Action;

typedef void (*p)(void);

p ClickCallBack[BUTTONn][N_ACTIONS] = 
{
	(p) BUTTON12_Click_Callback,	 	(p) BUTTON12_Retention_Callback

//	(p) BUTTON0_Click_Callback, 	// 	(p) BUTTON0_Retention_Callback
//	(p) BUTTON1_Click_Callback,		// 	(p) BUTTON1_Retention_Callback
//	(p) BUTTON2_Click_Callback,		// 	(p) BUTTON2_Retention_Callback
//	(p) BUTTON3_Click_Callback,		// 	(p) BUTTON3_Retention_Callback
//	(p) BUTTON4_Click_Callback,		// 	(p) BUTTON4_Retention_Callback
//	(p) BUTTON5_Click_Callback,		// 	(p) BUTTON5_Retention_Callback
//	(p) BUTTON6_Click_Callback,		// 	(p) BUTTON6_Retention_Callback
//	(p) BUTTON7_Click_Callback,		// 	(p) BUTTON7_Retention_Callback
//	(p) BUTTON8_Click_Callback,		// 	(p) BUTTON8_Retention_Callback
//	(p) BUTTON9_Click_Callback,		// 	(p) BUTTON9_Retention_Callback	
//	(p) BUTTON10_Click_Callback,	// 	(p) BUTTON10_Retention_Callback
//	(p) BUTTON11_Click_Callback,	// 	(p) BUTTON11_Retention_Callback
//	(p) BUTTON12_Click_Callback,	 	(p) BUTTON12_Retention_Callback
//	(p) BUTTON13_Click_Callback,	// 	(p) BUTTON13_Retention_Callback
//	(p) BUTTON14_Click_Callback,	// 	(p) BUTTON14_Retention_Callback
//	(p) BUTTON15_Click_Callback,		(p) BUTTON15_Retention_Callback
};

//p RetentionCallBack[BUTTONn] = 
//{
//	(p) BUTTON15_Click_Callback,
//}
/* Private variables ---------------------------------------------------------*/	

GPIO_TypeDef* BUTTON_PORT[BUTTONn] = { GPIOA };
const uint16_t BUTTON_PIN[BUTTONn] = { LL_GPIO_PIN_12 };
BUTTON_t BUTTON[BUTTONn];

/* Exported variables --------------------------------------------------------*/
extern unsigned long SystemCounter;

/* Public variables ----------------------------------------------------------*/
u16 EventCounter;	// Удалить
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*********************************************************************************
  * @brief    
  * @param  None
  * @retval None
*********************************************************************************/
void BUTTON_Init (void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);	//Clocking
	
	GPIO_InitStruct.Pin = LL_GPIO_PIN_12;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	BUTTON[BUTTON12].Tigger = 0;
	memset( (void*)&BUTTON, 0, sizeof(BUTTON_t) );
}


/**********************************************************************
  * @brief  Поток управления кнопками.
  * Вызывать в прерывании SysTick каждую 1 мс!!
  * @param  None
  * @retval None
  ********************************************************************/
void BUTTON_Processing(void)
{
	static u8 i;
	
	for(i = 0; i < BUTTONn; i++)
	{	
		if ( (BUTTON_PORT[i]->IDR & BUTTON_PIN[i]) == 0)
		{
			BUTTON[i].DebounceCounter++;
			
			if ( (BUTTON[i].DebounceCounter > BOUNCE_TIME) & (BUTTON[i].Tigger == 0) )
			{	// Срабатывание зарегистрировано
				
				ClickCallBack[i][CLICK]();
				BUTTON[i].Tigger = 1;	
			}
			else if (BUTTON[i].Tigger == 1)
			{
				BUTTON[i].RetentionCounter++;
				
				if(BUTTON[i].RetentionCounter > RETENTION_TIME)
				{	// Удержание зарегистрировано
					
					ClickCallBack[i][RETENTION]();
					BUTTON[i].RetentionCounter = 0;
					BUTTON[i].Tigger = 2;
				}
			}
			else if (BUTTON[i].Tigger == 2)
			{
				BUTTON[i].RetentionCounter++;
				
				if(BUTTON[i].RetentionCounter > REPEAT_TIME)
				{
					ClickCallBack[i][RETENTION]();
					BUTTON[i].RetentionCounter = 0;
				}
			}
		}
		else
		{
			BUTTON[i].DebounceCounter = 0;
			BUTTON[i].RetentionCounter = 0;
			BUTTON[i].Tigger = 0;
		}
	}
}


void BUTTON12_Click_Callback(void)
{
	LED_TurnOn(LED1, 1);
}

void BUTTON12_Retention_Callback(void)
{
	LED_TurnOn(LED1, 5);
}




























