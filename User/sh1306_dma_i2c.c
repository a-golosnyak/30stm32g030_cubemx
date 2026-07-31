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
#include "power_module.h"
#include "ds18b20_usart.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

OLED_t OLED;
char RenderBuffer[25];

/* Exported variables --------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void I2C_LowLevel_Init(void);
void I2C_Start (void);
void I2C_Stop (void);
u8 I2C_Write_Transaction (u8 Adress, u8 Register, u8 *Data, u8 Size);
u8 I2C_Read_Transaction (u8 Adress, u8 Register, u8 *Data, u8 Size);

u8 OLED_WriteCommand(uint8_t data);
u8 OLED_WriteCommandAsync(uint8_t* data, uint16_t Size);
u8 OLED_WriteData(uint8_t* data, uint16_t Size);
u8 OLED_WriteDataAsync(uint8_t command, uint8_t* data, uint16_t Size);
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
	uint8_t oledInitCommands[] = {
	    0xAE, // Display OFF
	    0x20, // Set Memory Addressing Mode
	    0x10, // Page Addressing Mode (RESET)
	    0xB0, // Set Page Start Address for Page Addressing Mode (0-7)
	    0xC8, // Set COM Output Scan Direction
	    0x00, // Set Low Column Address
	    0x10, // Set High Column Address
	    0x40, // Set Start Line Address
	    0x81, // Set Contrast Control Register
	    0xCF, // Contrast Value (MAX)
	    0xA1, // Set Segment Re-map 0 to 127
	    0xA6, // Set Normal Display
	    0xA8, // Set Multiplex Ratio (1 to 64)
	    0x3F, //
	    0xA4, // Output follows RAM content
	    0xD3, // Set Display Offset
	    0x00, // No Offset
	    0xD5, // Set Display Clock Divide Ratio/Oscillator Frequency
	    0x80, // Set Divide Ratio
	    0xD9, // Set Pre-charge Period
	    0x22, //
	    0xDA, // Set COM Pins Hardware Configuration
	    0x12, //
	    0xDB, // Set VCOMH
	    0x20, // 0.77xVcc
	    0x8D, // Set DC-DC (Charge Pump) Enable
	    0x14, //
	    0xAF  // Turn ON Panel
	};

	/* Init LCD */
	while(OLED_WriteDataAsync(SH1306_COMMANDS_ARRAY, oledInitCommands, sizeof(oledInitCommands)) == 1) {}

	OLED_Clear(0);

	LCD_PutText(" Temp Module v1.0 Debug ", 0, 0, Tahoma8, 1, 1);
	LCD_DrawLine(0, 10, 127, 10, 1);

	sprintf(RenderBuffer, "SCLK=%dMHz  ", (int)RCC_Clocks.SYSCLK_Frequency/1000000);
	LCD_PutText(RenderBuffer, 64, 11, Tahoma8, 1, 1);
}

/*********************************************************************************
 * @brief  ����� ������															//
 * @param  None																	//
 * @retval None																	//
 *********************************************************************************/
void OLED_Processing(void)
{
	if (OLED.Counter < SystemCounter)
	{
		OLED.Counter = SystemCounter + 100;
//		LED_TurnOn(LED1, 1);
		u8 y = 0;
//		LED_On(LED1);

//		sprintf(RenderBuffer, "n=%d  ", (u8)sensorCount);
//		LCD_PutText(RenderBuffer, 0, y+=11, Tahoma8, 1, 1);

		y+=11;
		if(testTemp1 != testTemp1Old) {
			char sign = (testTemp1 < 0) ? '-' : ' ';
			if (testTemp1 < 0) testTemp1 = -testTemp1; 									// Работаем с модулем числа
			int16_t celsius = testTemp1 / 16;
			int16_t fraction = ((testTemp1 % 16) * 10) / 16; 							// Переводит "шестнадцатые" строго в десятые
			sprintf(RenderBuffer, "Tds1=%c%d.%d\x7F ", sign, celsius, fraction); 		// \x7F - degree sign °
			LCD_PutText(RenderBuffer, 64, y, Tahoma8, 1, 1);

			testTemp1Old = testTemp1;
		}

		if(PWRMNG.Vntc != PWRMNG.VntcOld) {
			int16_t t_celsius = PWRMNG.Vntc / 100;       								// Целая часть (например, 25)
			int16_t t_fraction = abs(PWRMNG.Vntc % 10); 								// Сотые доли (например, 50)

			sprintf(RenderBuffer, "Tntc= %d.%01d\x7F  ", t_celsius, t_fraction);
			LCD_PutText(RenderBuffer, 0, y, Tahoma8, 1, 1);

			PWRMNG.VntcOld = PWRMNG.Vntc;

		}
		//		sprintf(RenderBuffer, "SYSCLK=%d  ", (int)RCC_Clocks.SYSCLK_Frequency);
		//		LCD_PutText(RenderBuffer, 0, y+=11, Tahoma8, 1, 1);

//------------------------------------------------------------------------------------------------------------------------

//		sign = (testTemp2 < 0) ? '-' : ' ';
//		if (testTemp2 < 0) testTemp2 = -testTemp2; 									// Работаем с модулем числа
//		celsius = testTemp2 / 16;
//		fraction = ((testTemp2 % 16) * 10) / 16; 									// Переводит "шестнадцатые" строго в десятые
//		sprintf(RenderBuffer, "Tds2=%c%d.%d\x7F ", sign, celsius, fraction); 		// \x7F - degree sign °
//		LCD_PutText(RenderBuffer, 0, y+=11, Tahoma8, 1, 1);

//		sprintf(RenderBuffer, "Tmcu=%d.%d\x7F  ", PWRMNG.Temp/10, PWRMNG.Temp%10);
//		LCD_PutText(RenderBuffer, 64, y, Tahoma8, 1, 1);
//		y+=11;
//		if(PWRMNG.Vbat != PWRMNG.VbatOld) {
//			PWRMNG.VbatOld = PWRMNG.Vbat;
//			sprintf(RenderBuffer, "Vbat=%d.%03d    ", PWRMNG.Vbat/1000, PWRMNG.Vbat%1000);
//			LCD_PutText(RenderBuffer, 0, y, Tahoma8, 1, 1);
//		}
//---------- RCC ---------------------------------------------------------------------------------------------------------
//		sprintf(RenderBuffer, "SYSCLK=%d  ", (int)RCC_Clocks.SYSCLK_Frequency);
//		LCD_PutText(RenderBuffer, 0, y+=11, Tahoma8, 1, 1);
//
//		sprintf(RenderBuffer, "HCLK=    %d  ", (int)RCC_Clocks.HCLK_Frequency);
//		LCD_PutText(RenderBuffer, 0, y+=11, Tahoma8, 1, 1);
//
//		sprintf(RenderBuffer, "PCLK1=   %d  ", (int)RCC_Clocks.PCLK1_Frequency);
//		LCD_PutText(RenderBuffer, 0, y+=11, Tahoma8, 1, 1);

//------------------------------------------------------------------------------------------------------------------------

		y+=11;
		if(PWRMNG.Vref != PWRMNG.VrefOld) {
			PWRMNG.VrefOld = PWRMNG.Vref;
			sprintf(RenderBuffer, "Vref=%d.%03d  ", PWRMNG.Vref/1000, PWRMNG.Vref%1000);
			LCD_PutText(RenderBuffer, 0, y, Tahoma8, 1, 1);
		}

		if(PWRMNG.Vdda != PWRMNG.VddaOld) {
			PWRMNG.VddaOld = PWRMNG.Vdda;
			sprintf(RenderBuffer, "Vdda=%d.%03d  ", PWRMNG.Vdda/1000, PWRMNG.Vdda%1000);
			LCD_PutText(RenderBuffer, 64, y, Tahoma8, 1, 1);
		}

		y+=11;
		if(PWRMNG.Vshunt != PWRMNG.VshuntOld) {
			PWRMNG.VshuntOld = PWRMNG.Vshunt;
			sprintf(RenderBuffer, "Vin= %d.%03d    ", PWRMNG.Vshunt/1000, PWRMNG.Vshunt%1000);
			LCD_PutText(RenderBuffer, 0, y, Tahoma8, 1, 1);
		}

		s16 K = PWRMNG.VopAmp/PWRMNG.Vshunt;
		sprintf(RenderBuffer, "K=%d  ", K);
		LCD_PutText(RenderBuffer, 64, y, Tahoma8, 1, 1);

		y+=11;
		if(PWRMNG.VopAmp != PWRMNG.VopAmpOld) {
			PWRMNG.VopAmpOld = PWRMNG.VopAmp;
			sprintf(RenderBuffer, "Vou=%d.%03d    ", PWRMNG.VopAmp/1000, PWRMNG.VopAmp%1000);
			LCD_PutText(RenderBuffer, 0, y, Tahoma8, 1, 1);
		}

		s16 I = PWRMNG.VopAmp*100/143;
		sprintf(RenderBuffer, "I=%d.%03dA  ", I/1000, I%1000);
		LCD_PutText(RenderBuffer, 64, y, Tahoma8, 1, 1);


		OLED.flag.sendData = 1;
//		LED_Off(LED1);
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
	static uint8_t commands[5];

	switch(OLED.MainStateMachine)
	{
		case 0:
			if(OLED.flag.sendData == 1) {
				OLED.flag.sendData = 0;
				x = 0;
				y = 0;
				OLED.MainStateMachine++;
//				LED_On(LED1);
			}
		break;

		case 1:
			while (y < 8) {
				if (OLED.glcd_dirty_pages & (1u << y)) {
					break;
				}
				y++;
			}

			if (y >= 8) {
//				LED_Off(LED1);
				OLED.glcd_dirty_pages = 0;
				OLED.MainStateMachine = 0;
				break;
			}

			if(!LL_I2C_IsActiveFlag_BUSY(I2C1))
			{
				commands[0] = SH1106_PAGEADDR | y;
				commands[1] = SH1106_SETLOWCOLUMN | (x);
				commands[2] = SH1106_SETHIGHCOLUMN;
				OLED.MainStateMachine++;
			}
		break;

		case 2:
			if(OLED_WriteDataAsync(SH1306_COMMANDS_ARRAY, commands, 3) == 0) {
				OLED.MainStateMachine++;
			}
		break;

		case 3:
			if(!LL_I2C_IsActiveFlag_BUSY(I2C1))
			{
				OLED.MainStateMachine++;
			}
		break;

		case 4:
			if(OLED_WriteDataAsync(SH1306_DATA, &OLED.Buffer[y*128], 128) == 0) {
				OLED.glcd_dirty_pages &= ~(1u << y); // страница отправлена
				OLED.MainStateMachine++;
			}
		break;

		case 5:
			y++;
			if(y<8) {
				OLED.MainStateMachine = 1;
			} else {
				OLED.MainStateMachine++;
			}
		break;

		case 6:
//			LED_Off(LED1);
			OLED.glcd_dirty_pages = 0;
			OLED.MainStateMachine = 0;
		break;

		default:
			OLED.MainStateMachine = 0;
		break;
	}
}

/*********************************************************************************
 * @brief
 * @param  None
 * @retval None
 *********************************************************************************/
void PutPixel(uint8_t x, uint8_t y, uint8_t value)
{
	if (x > SCREEN_WIDTH || y > SCREEN_HEIGHT)
			return;

	uint16_t page = y / 8;
	uint16_t array_pos = x + (page * 128);

	OLED.glcd_dirty_pages |= (1u << page);   // помечаем страницу как грязную

	if (value)
		OLED.Buffer[array_pos] |= (1u << (y % 8));
	else
		OLED.Buffer[array_pos] &= ~(1u << (y % 8));
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
 * @brief  Асинхронная отправка данных
 * @param  None
 * @retval None
 *********************************************************************************/
u8 OLED_WriteDataAsync(u8 command, uint8_t* data, uint16_t Size)
{
    uint8_t status = 0;

    switch (OLED.sendDataStateMachine) {
    case 0:
        LL_I2C_ClearFlag_NACK(I2C1);
        LL_DMA_ClearFlag_TC1(DMA1);
        LL_DMA_ClearFlag_TE1(DMA1);
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
        LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)data);
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, Size);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);

        LL_I2C_HandleTransfer(I2C1, SH1306_ADDR, LL_I2C_ADDRSLAVE_7BIT,
                             Size + 1, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

        OLED.sendDataCounter = SystemCounter + 10;
        OLED.sendDataStateMachine++;
        break;

    case 1:
        if (LL_I2C_IsActiveFlag_TXE(I2C1) || OLED.sendDataCounter < SystemCounter) {
            LL_I2C_TransmitData8(I2C1, command);
            LL_I2C_EnableDMAReq_TX(I2C1);
            OLED.sendDataCounter = SystemCounter + 10;
            OLED.sendDataStateMachine++;
        }
        break;

    case 2:
        if (LL_I2C_IsActiveFlag_STOP(I2C1) || LL_I2C_IsActiveFlag_NACK(I2C1) || (OLED.sendDataCounter < SystemCounter))
        {
            status = LL_I2C_IsActiveFlag_NACK(I2C1);
            LL_I2C_DisableDMAReq_TX(I2C1);
            LL_I2C_ClearFlag_STOP(I2C1);
            OLED.sendDataStateMachine++;
        }
        break;

    case 3:
        if (status == 0) {
            OLED.sendDataStateMachine = 0;
            return 0;
        }
        OLED.sendDataStateMachine = 0;
        break;

    default:
        OLED.sendDataStateMachine = 0;
        break;
    }
    return 1;
}


/*********************************************************************************
 * @brief  ������� 1 ���� �������.
 * @param  None
 * @retval None
 *********************************************************************************/
u8 OLED_WriteCommand(uint8_t data)
{
	static uint8_t tx_data[2];
	tx_data[0] = 0x80;
	tx_data[1] = data; // Разыменовываем указатель
	uint8_t status = 0;

	do {
	    LL_I2C_ClearFlag_NACK(I2C1);
	    LL_DMA_ClearFlag_TC1(DMA1);
	    LL_DMA_ClearFlag_TE1(DMA1);

	    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
	    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)tx_data);
	    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, 2);
	    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);

	    LL_I2C_HandleTransfer(I2C1, SH1306_ADDR, LL_I2C_ADDRSLAVE_7BIT, 2, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);
	    LL_I2C_EnableDMAReq_TX(I2C1);

	    uint32_t start = SystemCounter + 10;
	    while(!LL_I2C_IsActiveFlag_STOP(I2C1) && !LL_I2C_IsActiveFlag_NACK(I2C1) && (start < SystemCounter)) {}

	    status = LL_I2C_IsActiveFlag_NACK(I2C1);
	    LL_I2C_DisableDMAReq_TX(I2C1);
	    LL_I2C_ClearFlag_STOP(I2C1);

	} while(status == 1);

	return 0;
}

/*********************************************************************************
 * @brief  ������� 1 ���� ������.
 * @param  None
 * @retval None
 *********************************************************************************/
u8 OLED_WriteData(uint8_t* data, uint16_t Size)
{
	static uint8_t tx_data[129];
	tx_data[0] = 0x40;
	memcpy(&tx_data[1], data, Size);
	uint8_t status;

	do {
	    LL_I2C_ClearFlag_NACK(I2C1);

	    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
	    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)tx_data);

	    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, Size + 1);
	    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);

	    LL_I2C_HandleTransfer(I2C1, SH1306_ADDR, LL_I2C_ADDRSLAVE_7BIT, Size + 1, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);
	    LL_I2C_EnableDMAReq_TX(I2C1);

	    uint32_t start = SystemCounter + 10;
	    while(!LL_I2C_IsActiveFlag_STOP(I2C1) && !LL_I2C_IsActiveFlag_NACK(I2C1) && (start < SystemCounter)) {}

	    status = LL_I2C_IsActiveFlag_NACK(I2C1);
	    LL_I2C_DisableDMAReq_TX(I2C1);
	    LL_I2C_ClearFlag_STOP(I2C1);

	} while(status == 1);

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








