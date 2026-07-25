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
#ifndef __graphics5_H
#define __graphics5_H

/* Includes ------------------------------------------------------------------*/

#include <sh1306_dma_i2c.h>
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct 
{
	unsigned char x1;
	unsigned char y1; 
	unsigned char x2; 
	unsigned char y2;
} bounding_box_t;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
bounding_box_t LCD_PutCharLatin(unsigned char c, unsigned char x, unsigned char y, const unsigned char *font, char colour);
bounding_box_t LCD_PutText(char *string, unsigned char x, unsigned char y, const unsigned char *font, unsigned char spacing, char colour);
bounding_box_t LCD_PutCharCyrill(u8 c, u8 x, u8 y, const unsigned char *font, char colour);
void LCD_DrawLine(int x1, int y1, int x2, int y2, char colour);
void LCD_DrawRectangle(int x1, int y1, int x2, int y2, char colour);
void LCD_DrawRoundedRectangle(int x1, int y1, int x2, int y2, char colour);
void LCD_DrawCircle(unsigned char centre_x, unsigned char centre_y, unsigned char radius, unsigned char colour);
void LCD_DrawFilledCircle(unsigned char centre_x, unsigned char centre_y, unsigned char radius, unsigned char colour);
void LCD_DrawLineX(int x, int y1, int y2, char colour);
void LCD_DrawLineY(int x1, int x2, int y, char colour);
void LCD_DrawFilledRectangle(int x1, int y1, int x2, int y2, char colour);
	
#endif		//__graphics5_H
/******************END OF FILE*************************************************/











