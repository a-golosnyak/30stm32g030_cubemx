/**
  ******************************************************************************
  * @file    Led.c
  * @author  
  * @version 
  * @date    
  * @brief   
  ******************************************************************************
  * @attention

  ******************************************************************************
  */
	
/* Includes ------------------------------------------------------------------*/
	
#include "main.h"
#include "led_module.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/	

#define LEDn	2

GPIO_TypeDef* GPIO_PORT[LEDn] = {	GPIOB, GPIOA };

const uint16_t GPIO_PIN[LEDn] = { 	GPIO_PIN_0, GPIO_PIN_4 };

LEDs_t LED[LEDn];

/* Exported variables --------------------------------------------------------*/	
/* Private function prototypes -----------------------------------------------*/

//void LED_On(Led_TypeDef Led);
//void LED_Off(Led_TypeDef Led);
//void LED_Toggle(Led_TypeDef Led);
		
/* Private functions ---------------------------------------------------------*/

/**********************************************************************
  * @brief  Инициализация светодиодов.
  * @param  None
  * @retval None
   ********************************************************************/
void LED_Init (void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	  /* GPIO Ports Clock Enable */
	  __HAL_RCC_GPIOB_CLK_ENABLE();
	  __HAL_RCC_GPIOA_CLK_ENABLE();

	  /*Configure GPIO pin Output Level */
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
	  /*Configure GPIO pin Output Level */
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

	  /*Configure GPIO pins : PB0 */
	  GPIO_InitStruct.Pin = GPIO_PIN_0;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	  /*Configure GPIO pin : PA4 */
	  GPIO_InitStruct.Pin = GPIO_PIN_4;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  LED_On(LED0);		// Inverted state.
}

/**********************************************************************
  * @brief  Включить светодиод.
  * @param  None
  * @retval None
   ********************************************************************/
void LED_On(Led_TypeDef Led)
{
  GPIO_PORT[Led]->BSRR = GPIO_PIN[Led];
}
/**********************************************************************
  * @brief  Выключить светодиод.
  * @param  None
  * @retval None
   ********************************************************************/
void LED_Off(Led_TypeDef Led)
{
  GPIO_PORT[Led]->BRR = GPIO_PIN[Led];  
}
/**********************************************************************
  * @brief  Переключить состояние светодиода.
  * @param  None
  * @retval None
   ********************************************************************/
void LED_Toggle(Led_TypeDef Led)
{
  GPIO_PORT[Led]->ODR ^= GPIO_PIN[Led];
}


/**********************************************************************
  * @brief  Включить светодиод на время
  * @param  led: тип светодиода
  *	@param  time_ms: время, которое светодиод будет включен
  * @retval None
  ********************************************************************/
void LED_TurnOn(Led_TypeDef led, u32 time_ms)
{
	LED_On(led);
	LED[led].Times = 1;
	LED[led].Counter = SystemCounter + time_ms;
}

/**********************************************************************
  * @brief  Включить мигание светодиода
  * @param  led: тип светодиода
  *	@param  time_on: время, которое светодиод будет включен
  *	@param  time_off: время, через которое будет переключено в противоположное состояние
  *	@param  times: сколько раз мигнуть
  * @retval None
  ********************************************************************/
void LED_TurnOn_Blink(Led_TypeDef led, u16 time_on, u16 time_off, u16 times)
{
	LED_On(led);
	LED[led].Counter = SystemCounter + time_on;
	LED[led].TimeOn = time_on;
	LED[led].TimeOff = time_off;
	LED[led].Times = (times*2)-1;
}

/**********************************************************************
  * @brief  Общая функция обработки состояния светодиодов (вызывать в цикле)
  * @param  None
  * @retval None
  ********************************************************************/
void LED_Processing(void)
{
	static u8 i;
	
	for (i = 0; i < LEDn; i++)
	{
		if (LED[i].Times != 0)
		{
			if ( (HAL_GPIO_ReadPin(GPIO_PORT[i], GPIO_PIN[i]) == GPIO_PIN_SET) && (SystemCounter > LED[i].Counter) )
			{
				LED_Off((Led_TypeDef)i);
				LED[i].Counter = SystemCounter + LED[i].TimeOff;
				LED[i].Times--;
			}	
			else if ( (HAL_GPIO_ReadPin(GPIO_PORT[i], GPIO_PIN[i]) == GPIO_PIN_RESET) && (SystemCounter > LED[i].Counter) )
			{
				LED_On((Led_TypeDef)i);
				LED[i].Counter = SystemCounter + LED[i].TimeOn;
				LED[i].Times--;
			}
		}		
	}
}
