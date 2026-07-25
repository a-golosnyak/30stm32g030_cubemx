/**
 ******************************************************************************
 * @file    sh1306_soft_i2c.c
 * @author
 * @version
 * @date
 * @brief
 ******************************************************************************
 * @attention

 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <ds18b20_usart.h>
#include "Fonts/font_tahoma8+.h"
#include <sh1306_dma_i2c.h>
#include "string.h"
#include "stdio.h"
#include <stdlib.h>

#include "led_module.h"
#include "graphics5.h"
#include "main.h"
#include "i2c.h"
#include "power_module.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

OLED_t OLED;
char RenderBuffer[20];

/* Exported variables --------------------------------------------------------*/
extern u32 SystemCounter;
/* Private function prototypes -----------------------------------------------*/
void I2C_LowLevel_Init(void);
void I2C_Start (void);
void I2C_Stop (void);
u8 I2C_Write_Transaction (u8 Adress, u8 Register, u8 *Data, u8 Size);
u8 I2C_Read_Transaction (u8 Adress, u8 Register, u8 *Data, u8 Size);

u8 OLED_WriteCommand(uint8_t data);
u8 OLED_WriteData(uint8_t* data, uint16_t Size);
u8 OLED_BurstWrite(uint8_t* pBuffer, uint8_t NumByteToWrite);
u8 I2C_TIMEOUT_UserCallback(u8 State);

/* Private functions ---------------------------------------------------------*/

/*********************************************************************************
 * @brief
 * @param  None
 * @retval None
 *********************************************************************************/
void OLED_Init(void)
{
	/* Init LCD */
	OLED_WriteCommand(0xAE); //display off
	OLED_WriteCommand(0x20); //Set Memory Addressing Mode
	OLED_WriteCommand(0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	OLED_WriteCommand(0xB0); //Set Page Start Address for Page Addressing Mode,0-7
	OLED_WriteCommand(0xC8); //Set COM Output Scan Direction
	OLED_WriteCommand(0x00); //---set low column address
	OLED_WriteCommand(0x10); //---set high column address
	OLED_WriteCommand(0x40); //--set start line address
	OLED_WriteCommand(0x81); //--set contrast control register
	OLED_WriteCommand(0xff);
	OLED_WriteCommand(0xA1); //--set segment re-map 0 to 127
	OLED_WriteCommand(0xA6); //--set normal display
	OLED_WriteCommand(0xA8); //--set multiplex ratio(1 to 64)
	OLED_WriteCommand(0x3F); //
	OLED_WriteCommand(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	OLED_WriteCommand(0xD3); //-set display offset
	OLED_WriteCommand(0x00); //-not offset
	OLED_WriteCommand(0xD5); //--set display clock divide ratio/oscillator frequency
	OLED_WriteCommand(0xF0); //--set divide ratio
	OLED_WriteCommand(0xD9); //--set pre-charge period
	OLED_WriteCommand(0x22); //
	OLED_WriteCommand(0xDA); //--set com pins hardware configuration
	OLED_WriteCommand(0x12);
	OLED_WriteCommand(0xDB); //--set vcomh
	OLED_WriteCommand(0x20); //0x20,0.77xVcc
	OLED_WriteCommand(0x8D); //--set DC-DC enable
	OLED_WriteCommand(0x14); //
	OLED_WriteCommand(0xAF); //--turn on SSD1306 panel

	OLED_Clear(0);

	LCD_PutText(" Temp Module v1.0 Debug ", 0, 0, Tahoma8, 1, 1);
	LCD_DrawLine(0, 10, 127, 10, 1);
}

/*********************************************************************************
 * @brief  ����� ������														//
 * @param  None																//
 * @retval None																//
 *********************************************************************************/
void OLED_Processing(void)
{
	if (HAL_GetTick() - OLED.Counter >= 100) {
		OLED.Counter = HAL_GetTick();
		u8 y = 0;

//		sprintf(RenderBuffer, "n=%d  ", (u8)sensorCount);
//		LCD_PutText(RenderBuffer, 0, y+=11, Tahoma8, 1, 1);

//		char sign = (testTemp1 < 0) ? '-' : ' ';
//		if (testTemp1 < 0) testTemp1 = -testTemp1; 									// Работаем с модулем числа
//		int16_t celsius = testTemp1 / 16;
//		int16_t fraction = ((testTemp1 % 16) * 10) / 16; 							// Переводит "шестнадцатые" строго в десятые
//		sprintf(RenderBuffer, "t1=%c%d.%d\x7F ", sign, celsius, fraction); 	// \x7F - degree sign °
//		LCD_PutText(RenderBuffer, 30, y, Tahoma8, 1, 1);
//
//		sign = (testTemp2 < 0) ? '-' : ' ';
//		if (testTemp2 < 0) testTemp2 = -testTemp2; 									// Работаем с модулем числа
//		celsius = testTemp2 / 16;
//		fraction = ((testTemp2 % 16) * 10) / 16; 									// Переводит "шестнадцатые" строго в десятые
//		sprintf(RenderBuffer, "t2=%c%d.%d\x7F ", sign, celsius, fraction); 	// \x7F - degree sign °
//		LCD_PutText(RenderBuffer, 82, y, Tahoma8, 1, 1);

		sprintf(RenderBuffer, "Vref=%d.%03d  ", PWRMNG.Vref/1000, PWRMNG.Vref%1000);
		LCD_PutText(RenderBuffer, 0, y+=11, Tahoma8, 1, 1);
		sprintf(RenderBuffer, "Vdda=%d  ", PWRMNG.Vdda);
		LCD_PutText(RenderBuffer, 70, y, Tahoma8, 1, 1);

		sprintf(RenderBuffer, "Vtemp=%d.%d\x7F  ", PWRMNG.Temp/10, PWRMNG.Temp%10);
		LCD_PutText(RenderBuffer, 0, y+=11, Tahoma8, 1, 1);
		sprintf(RenderBuffer, "%d   ", PWRMNG.AdcCod[TEMP]/16);
		LCD_PutText(RenderBuffer, 70, y, Tahoma8, 1, 1);

		int16_t t_celsius = PWRMNG.Vntc / 100;       // Целая часть (например, 25)
		int16_t t_fraction = abs(PWRMNG.Vntc % 100); // Сотые доли (например, 50)
//
		sprintf(RenderBuffer, "Temp: %d.%02d  ", t_celsius, t_fraction);
////		sprintf(RenderBuffer, "Vntc=%d  ", PWRMNG.Vntc);
		LCD_PutText(RenderBuffer, 0, y+=11, Tahoma8, 1, 1);
		sprintf(RenderBuffer, "%d  ", PWRMNG.AdcCod[VNTC]/16);
		LCD_PutText(RenderBuffer, 70, y, Tahoma8, 1, 1);
//
		sprintf(RenderBuffer, "Vbat=%d    ", PWRMNG.Vbat);
		LCD_PutText(RenderBuffer, 0, y+=11, Tahoma8, 1, 1);
//		sprintf(RenderBuffer, "%d    ", PWRMNG.AdcCod[VBAT]);
//		LCD_PutText(RenderBuffer, 70, y, Tahoma8, 1, 1);

//		uint16_t vrefint_cal = *VREFINT_CAL_ADDR;
//		sprintf(RenderBuffer, "Vcal=%d  ", vrefint_cal);
//		LCD_PutText(RenderBuffer, 0, y+=11, Tahoma8, 1, 1);

		OLED.flag.sendData = 1;
	}


	OLED_RenderAll();
}

/*********************************************************************************
 * @brief
 * @param  None
 * @retval None
 *********************************************************************************/
void OLED_Clear(u8 color)
{
	memset(OLED.Buffer, color, sizeof(OLED.Buffer));
	OLED.glcd_dirty_pages = 0xFF;
	OLED_RenderAll();
}

/*********************************************************************************
 * @brief
 * @param  None
 * @retval None
 *********************************************************************************/
void OLED_RenderAll(void)
{
	static int x = 0, y = 0;

	switch(OLED.MainStateMachine)
	{
		case 0:
			if(OLED.flag.sendData == 1) {
				OLED.flag.sendData = 0;
				x = 0;
				y = 0;
//				LED_On(LED1);
				OLED.MainStateMachine++;
			}
		break;

		case 1:
			if(HAL_I2C_GetState(&hi2c1) == HAL_I2C_STATE_READY)
			{
				OLED_WriteCommand(SH1106_PAGEADDR | y);
				OLED_WriteCommand(SH1106_SETLOWCOLUMN | (x));
				OLED_WriteCommand(SH1106_SETHIGHCOLUMN);

				OLED.MainStateMachine++;
			}
		break;

		case 2:
			if(HAL_I2C_GetState(&hi2c1) == HAL_I2C_STATE_READY)
			{
				OLED_WriteData(&OLED.Buffer[y*128], 128);
				y++;
				if(y<8) {
					OLED.MainStateMachine = 1;
				} else {
					OLED.MainStateMachine++;
				}
			}
		break;

		case 3:
//			LED_Off(LED1);
			OLED.glcd_dirty_pages = 0;
			OLED.MainStateMachine = 0;
		break;

		default:
			OLED.MainStateMachine = 0;
			break;
	}

//	static int x = 0, y = 0;
//
//	for (y = 0; y < 8; y++)
//	{
//		if (!(OLED.glcd_dirty_pages & (1 << y)))
//			continue;
//
//		OLED_WriteCommand(SH1106_PAGEADDR | y);
//		OLED_WriteCommand(SH1106_SETLOWCOLUMN | (x));
//		OLED_WriteCommand(SH1106_SETHIGHCOLUMN);
//
//		OLED_WriteData(&OLED.Buffer[y*128], 128);
//	}
//	OLED.glcd_dirty_pages = 0;
}

/*********************************************************************************
 * @brief
 * @param  None
 * @retval None
 *********************************************************************************/
void PutPixel(uint8_t x, uint8_t y, uint8_t value)
{
	unsigned short array_pos;

	array_pos = x + ((y / 8) * 128);
	OLED.glcd_dirty_pages |= 1 << (array_pos / 128);

	if (x > SCREEN_WIDTH || y > SCREEN_HEIGHT)
		return;

	if (value)
		OLED.Buffer[array_pos] |= (1 << (y % 8));
	else
		OLED.Buffer[array_pos] &= (0xFF ^ (1 << (y % 8)));
}

/*********************************************************************************
 * @brief  Inverse the screen, swapping "on" and "off" pixels.
 * 		This does not affect the RAM buffer or the screen memory, the controller
 * 		is capable of reversing pixels with a single command.
 * @param  None
 * @retval None
 *********************************************************************************/
void OLED_inverse_screen(unsigned char inverse) 
{
	if (inverse)
		OLED_WriteCommand(SH1106_INVERTDISPLAY);
	else
		OLED_WriteCommand(SH1106_NORMALDISPLAY);
}

/*********************************************************************************
 * @brief  ������� 1 ���� �������.
 * @param  None
 * @retval None
 *********************************************************************************/
u8 OLED_WriteCommand(uint8_t data)
{
	do{
		uint8_t tx_data[2] = {0x80, data};
		HAL_I2C_Master_Transmit_DMA(&hi2c1, (uint16_t)SH1306_ADDR, (uint8_t *)tx_data, 2);

		while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
		{
		}
	}
	while(HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF);

	return 0;
}
/*********************************************************************************
 * @brief  ������� 1 ���� ������.
 * @param  None
 * @retval None
 *********************************************************************************/
u8 OLED_WriteData(uint8_t* data, uint16_t Size)
{
	do
	{
		static uint8_t tx_data[129];

		tx_data[0] = 0x40;
		memcpy(&tx_data[1], data, 128);

		if (HAL_I2C_Master_Transmit_DMA(&hi2c1, (uint16_t)SH1306_ADDR, tx_data, Size+1) != HAL_OK)
		{
			Error_Handler();
		}

		while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
		{
		}
	}
	while (HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF);

	return 0;
}


/*********************************************************************************
 * @brief
 * @param  None
 * @retval None
 *********************************************************************************/
u8 I2C_TIMEOUT_UserCallback(u8 State)
{
//	LED_TurnON_Blink(LED12, 20, 100, 3);
	return State;
}






















/*
// ���������� ��������� ��� 

void OLED_Processing(void)
{
	if(SystemCounter > OLED.Counter)
	{
		OLED.Counter = SystemCounter + 80;
	
//		memset(OLED.Buffer, 0x00, sizeof(OLED.Buffer));			// ������� �����������.
		sprintf(RenderBuffer, "OLED.Counter = %2d.%1ds ", OLED.Counter/1000, OLED.Counter%10);
		LCD_PutText(RenderBuffer, 5, 0, Tahoma8, 1, 1);
		sprintf(RenderBuffer, "Stat = %x ", AD7799_Reg.StatusReg);
		LCD_PutText(RenderBuffer, 0, 12, Tahoma8, 1, 1);
		sprintf(RenderBuffer, "Mode = %x ", AD7799_Reg.ModeReg);
		LCD_PutText(RenderBuffer, 0, 22, Tahoma8, 1, 1);
		sprintf(RenderBuffer, "Conf = %x ", AD7799_Reg.ConfigReg);
		LCD_PutText(RenderBuffer, 0, 32, Tahoma8, 1, 1);
		sprintf(RenderBuffer, "Id = %x ", AD7799_Reg.IdReg);
		LCD_PutText(RenderBuffer, 0, 42, Tahoma8, 1, 1);
		
		sprintf(RenderBuffer, "   Cod = %d         ", (int)Cod/10);
		LCD_PutText(RenderBuffer, 0, 54, Tahoma8, 1, 1);
		
		sprintf(RenderBuffer, "Data = %d  ", AD7799.SDADC.Cod/100);
		LCD_PutText(RenderBuffer, 66, 12, Tahoma8, 1, 1);
		sprintf(RenderBuffer, "Io = %x ", AD7799_Reg.IoReg);
		LCD_PutText(RenderBuffer, 66, 22, Tahoma8, 1, 1);
		sprintf(RenderBuffer, "Offset = %x ", AD7799_Reg.OffsetReg);
		LCD_PutText(RenderBuffer, 66, 32, Tahoma8, 1, 1);
		sprintf(RenderBuffer, "Fscale = %x ", AD7799_Reg.FullscaleReg);
		LCD_PutText(RenderBuffer, 66, 42, Tahoma8, 1, 1);
				
		LCD_DrawLine(0, 10, 127, 10, 1);
		LCD_DrawLineY(63, 11, 52, 1);
		OLED_RenderAll();
	}
}*/








