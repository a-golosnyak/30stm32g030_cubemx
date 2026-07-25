/**
  ******************************************************************************
  * @file    sh1306_soft_i2c.h
  * @author  
  * @version 
  * @date    
  * @brief   
  ******************************************************************************
  * @attention

  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __Sh1306_Soft_I2c_H
#define __Sh1306_Soft_I2c_H

/* Includes ------------------------------------------------------------------*/

#include "main.h"

/* Private typedef -----------------------------------------------------------*/

typedef struct
{
	u8	sendData	:1;
} Flag_t;

/* Private define ------------------------------------------------------------*/
#define I2C_BUS I2C1

#define I2C_OFFSET_TIMINGR_SCLL		0
#define I2C_OFFSET_TIMINGR_SCLH		8
#define I2C_OFFSET_TIMINGR_SDADEL	16
#define I2C_OFFSET_TIMINGR_SCLDEL	20
#define I2C_OFFSET_TIMINGR_PRESC	28
#define I2C_OFFSET_CR2_NBYTES		16
//---------------------------------------------

#define FLAG_TIMEOUT         ((uint32_t)0x1000)
#define SH1306_ADDR           0x78   			// sh1306 address


#define SH1106_SETCONTRAST    		0x81
#define SH1106_DISPLAYALLON_RESUME 	0xA4
#define SH1106_DISPLAYALLON   		0xA5
#define SH1106_NORMALDISPLAY  		0xA6
#define SH1106_INVERTDISPLAY  		0xA7
#define SH1106_DISPLAYOFF     		0xAE
#define SH1106_DISPLAYON 	  		0xAF
#define SH1106_SETDISPLAYOFFSET 	0xD3
#define SH1106_SETCOMPINS 			0xDA
#define SH1106_SETVCOMDETECT 		0xDB
#define SH1106_SETDISPLAYCLOCKDIV 	0xD5
#define SH1106_SETPRECHARGE 		0xD9
#define SH1106_SETMULTIPLEX 		0xA8
#define SH1106_SETLOWCOLUMN 		0x00
#define SH1106_SETHIGHCOLUMN 		0x10
#define SH1106_PAGEADDR   			0xB0
#define SH1106_SETSTARTLINE 		0x40
#define SH1106_MEMORYMODE 			0x20
#define SH1106_COLUMNADDR 			0x21
#define SH1106_COMSCANINC 			0xC0
#define SH1106_COMSCANDEC 			0xC8
#define SH1106_SEGREMAP 	   		0xA0
#define SH1106_CHARGEPUMP 			0x8D
#define SH1106_EXTERNALVCC 			0x1
#define SH1106_SWITCHCAPVCC 		0x2

// Scrolling #defines
#define SH1106_ACTIVATE_SCROLL 						0x2F
#define SH1106_DEACTIVATE_SCROLL					0x2E
#define SH1106_SET_VERTICAL_SCROLL_AREA 			0xA3
#define SH1106_RIGHT_HORIZONTAL_SCROLL 				0x26
#define SH1106_LEFT_HORIZONTAL_SCROLL 				0x27
#define SH1106_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SH1106_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 	0x2A

#define SCREEN_WIDTH	127
#define SCREEN_HEIGHT	63

/* Private macro -------------------------------------------------------------*/
/* Public variables ----------------------------------------------------------*/	
typedef struct
{
	u32	Counter;
	u8	MainStateMachine;
	u8	Buffer[128 * 64 / 8];
	u8	glcd_dirty_pages;
	u32 Timeout;
	Flag_t flag;
}OLED_t;

extern char RenderBuffer[20];

extern uint8_t sensorCount;
extern int16_t testTemp;

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void OLED_Init (void);
void OLED_Clear(u8 color);
void OLED_RenderAll(void);
void OLED_inverse_screen(unsigned char inverse);
void PutPixel(uint8_t x, uint8_t y, uint8_t value);

void OLED_Processing(void);

#endif /*__Sh1306_Soft_I2c_H */



/******************END OF FILE*************************************************/









