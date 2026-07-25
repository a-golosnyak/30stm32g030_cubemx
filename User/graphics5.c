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
#include "graphics5.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
//--- Cirillyc --------------------------------------
#define FONT_HEADER_WIDTH		2
#define FONT_HEADER_HEIGHT		3
#define FONT_HEADER_START		4
#define FONT_HEADER_LETTERS		5

//--- Latin -----------------------------------------
#define FONT_HEADER_TYPE_LAT					0
#define FONT_HEADER_ORIENTATION_LAT				1
#define FONT_HEADER_START_LAT					2
#define FONT_HEADER_LETTERS_LAT					3
#define FONT_HEADER_HEIGHT_LAT					4	
#define FONT_TYPE_FIXED_LAT						0
#define FONT_TYPE_PROPORTIONAL_LAT				1
#define FONT_ORIENTATION_VERTICAL_CEILING_LAT	2
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/	
/* Exported variables --------------------------------------------------------*/						
/* Private function prototypes -----------------------------------------------*/
bounding_box_t LCD_PutCharLatin(unsigned char c, unsigned char x, unsigned char y, const unsigned char *font, char colour);
bounding_box_t LCD_PutTextLatin(char *string, unsigned char x, unsigned char y, const unsigned char *font, unsigned char spacing, char colour);
bounding_box_t LCD_PutCharCyrill(u8 chr, u8 X, u8 Y, const unsigned char *font, char colour);
bounding_box_t LCD_PutTextCyrill(char *string, unsigned char x, unsigned char y, const unsigned char *font, unsigned char spacing, char colour);

/* Private functions ---------------------------------------------------------*/
/*********************************************************************************
  * @brief  
  * @param  None
  * @retval None
*********************************************************************************/
bounding_box_t LCD_PutCharLatin(unsigned char c, unsigned char x, unsigned char y, const unsigned char *font, char colour)
{
	unsigned short pos;
	unsigned char width;
	bounding_box_t ret;
	unsigned char i;
	unsigned char j;
	
	ret.x1 = x;
	ret.y1 = y;
	ret.x2 = x;
	ret.y2 = y;

	if (font[FONT_HEADER_TYPE_LAT] != FONT_TYPE_PROPORTIONAL_LAT) 
		return ret;

	if (font[FONT_HEADER_ORIENTATION_LAT] != FONT_ORIENTATION_VERTICAL_CEILING_LAT)
		return ret;

	if (!(c >= font[FONT_HEADER_START_LAT] && c <= font[FONT_HEADER_START_LAT] + font[FONT_HEADER_LETTERS_LAT]))
		return ret;

	c -= font[FONT_HEADER_START_LAT];
	pos = font[c * FONT_HEADER_START_LAT + 5];
	pos <<= 8;
	pos |= font[c * FONT_HEADER_START_LAT + 6];
	width = font[pos];
	
	for (i = 0; i < width; i++) 
	{
		for (j = 0; j < font[FONT_HEADER_HEIGHT_LAT]; j++)
		{
			if (j % 8 == 0) 
				pos++;

			if (font[pos] & 1 << (j % 8))
				PutPixel(x + i, y + j, colour);
			else 
				PutPixel(x + i, y + j, !colour);
		}
	}
	ret.x2 = ret.x1 + width - 1;
	ret.y2 = ret.y1 + font[FONT_HEADER_HEIGHT_LAT];

	return ret;
}

/*********************************************************************************
  * @brief  
  * @param  None
  * @retval None
*********************************************************************************/
bounding_box_t LCD_PutTextLatin(char *string, unsigned char x, unsigned char y, const unsigned char *font, unsigned char spacing, char colour) 
{
	bounding_box_t ret;
	bounding_box_t tmp;
	char i;

	ret.x1 = x;
	ret.y1 = y;

	spacing += 1;

	// BUG: As we move right between chars we don't actually wipe the space
	while (*string != 0) 
	{
		tmp = LCD_PutCharLatin(*string++, x, y, font, colour);

		// Leave a single space between characters
		x = tmp.x2 + spacing;
		
		for(i = tmp.x2; i < x-1; i++)					
			LCD_DrawLineY(i+1, tmp.y1, tmp.y2-1, !colour);		// Вытираем пробелы между символами
		
		if(x >= SCREEN_WIDTH)									// Перенос строки
		{
			x = 0;
			y += font[FONT_HEADER_HEIGHT];
		}
	}

	ret.x2 = tmp.x2;
	ret.y2 = tmp.y2;

	return ret;
}


/*********************************************************************************
  * @brief  Вывод кириллического символа
  * @param  None
  * @retval None
*********************************************************************************/
bounding_box_t LCD_PutCharCyrill(u8 chr, u8 X, u8 Y, const unsigned char *font, char colour)
{
	s16 Index;
	unsigned char width=0;
	bounding_box_t ret;
	unsigned char x=0;
	unsigned char y=0;
	
	unsigned char i=0;
	unsigned char j=0;
	u8 shift;
		
	ret.x1 = X;
	ret.y1 = Y;
	ret.x2 = X;
	ret.y2 = Y;

	if(chr > 191)
		chr = chr - 127;
		
	if (!(chr >= font[FONT_HEADER_START] && chr <= font[FONT_HEADER_START] + font[FONT_HEADER_LETTERS]) ) 
		return ret;

	j = chr - font[FONT_HEADER_START] + 6;						// Место в массиве, где лежит ширина нужного символа
	Index = 6;													// Пропускаем первые 6 сервисных байт
	
	if (font[FONT_HEADER_HEIGHT] <= 8)							// Если шрифт одноэтажный (высота не более 1 байта
		for(i=6; i<j; i++)
			Index = Index + font[i];							// Суммируем ширину всех предидущих нужному символов
	else if (font[FONT_HEADER_HEIGHT] <= 16)					// Если шрифт двохетажный
		for(i=6; i<j; i++)
			Index = Index + font[i]*2;							// Суммируем ширину всех предидущих нужному символов
	else if	(font[FONT_HEADER_HEIGHT] <= 24)					// Если шрифт трехетажный
		for(i=6; i<j; i++)
			Index = Index + font[i]*3;							// Суммируем ширину всех предидущих нужному символов
	else														// Если шрифт четырехэтажный
		for(i=6; i<j; i++)
			Index = Index + font[i]*4;							// Суммируем ширину всех предидущих нужному символов
	
	Index = Index + font[FONT_HEADER_LETTERS];  				// Заканчиваем вычисления места положения нужного символа
	width = font[chr - font[FONT_HEADER_START] + 6];			// Ширина выводимого символа
	j = font[FONT_HEADER_HEIGHT]/8;								// Количество байт в высоту
	
	for(i = 0; i < j; i++)											// Выводим полные байты высоты (этажи)
	{
		for (x = 0; x < width; x++) 
		{
			for (y = 0; y < 8; y++) 
				if (font[Index] & 1 << (y % 8)) 
					PutPixel(X + x, Y + y + i*8, colour);
				else
					PutPixel(X + x, Y + y + i*8, !colour);
			Index++;
		}
	}

	for (x = 0; x < width; x++) 									// Выводим неполный последний байт высоты (этаж)
	{																// В нем порядок следования бит инвертирован
		for (y = 0; y < font[FONT_HEADER_HEIGHT]%8; y++) 			// т.е. старшие биты отображаются
		{
			shift = 0x80 >> ((font[FONT_HEADER_HEIGHT]%8) -1);
			
			if (font[Index] & shift << (y % 8)) 
				PutPixel(X + x, Y + y + i*8, colour);
			else
				PutPixel(X + x, Y + y + i*8, !colour);
		}
		Index++;
	}

	ret.x2 = ret.x1 + width-1;
	ret.y2 = ret.y1 + font[FONT_HEADER_HEIGHT];

	return ret;
}

/*********************************************************************************
  * @brief  Вывод кириллического текста
  * @param  None
  * @retval None
*********************************************************************************/
bounding_box_t LCD_PutTextCyrill(char *string, unsigned char x, unsigned char y, const unsigned char *font, unsigned char spacing, char colour) 
{
	bounding_box_t ret;
	bounding_box_t tmp;
	char i;
	
	ret.x1 = x;
	ret.y1 = y;

	spacing += 1;

	// BUG: As we move right between chars we don't actually wipe the space
	while (*string != 0) 
	{
		tmp = LCD_PutCharCyrill(*string++, x, y, font, colour);
		// Leave a single space between characters
		x = tmp.x2 + spacing;
		
		for(i = tmp.x2; i < x-1; i++)					
			LCD_DrawLineY(i+1, tmp.y1, tmp.y2-1, !colour);				// Вытираем пробелы между символами
		
		if(x >= SCREEN_WIDTH)
		{
			x = 0;
			y += font[FONT_HEADER_HEIGHT];
		}
	}

	ret.x2 = tmp.x2;
	ret.y2 = tmp.y2;

	return ret;
}


/********************************************************************************************
  * @brief  
  * @param  None
  * @retval None
*********************************************************************************************/
bounding_box_t LCD_PutText(char *string, unsigned char x, unsigned char y, const unsigned char *font, unsigned char spacing, char colour)
{
	bounding_box_t ret;
	bounding_box_t tmp;
	char i;

	ret.x1 = x;
	ret.y1 = y;

	spacing += 1;

	// BUG: As we move right between chars we don't actually wipe the space
	while (*string != 0) 
	{
		if (*string <= 127)
			tmp = LCD_PutCharLatin(*string++, x, y, font, colour);
		else
			tmp = LCD_PutCharCyrill(*string++, x, y, font, colour);

		// Leave a single space between characters
		x = tmp.x2 + spacing;
		
		for(i = tmp.x2; i < x-1; i++)					
			LCD_DrawLineY(i+1, tmp.y1, tmp.y2-1, !colour);		// Вытираем пробелы между символами
		
		if(x >= SCREEN_WIDTH)									// Перенос строки
		{
			x = 0;
			y += font[FONT_HEADER_HEIGHT];
		}
	}

	ret.x2 = tmp.x2;
	ret.y2 = tmp.y2;

	return ret;
	
}


/*********************************************************************************
  * @brief  Implementation of Bresenham's line algorithm
  * @param  None
  * @retval None
*********************************************************************************/
void LCD_DrawLine(int x1, int y1, int x2, int y2, char colour)
{
	int xinc1, yinc1, den, num, numadd, numpixels, curpixel, xinc2, yinc2;

	int deltax = x2 - x1;    		// The difference between the x's
	int deltay = y2 - y1;    		// The difference between the y's
	int x = x1;                   	// Start x off at the first pixel
	int y = y1;                   	// Start y off at the first pixel
	
	if(deltax < 0)
		deltax = -deltax;
	
	if(deltay < 0)
		deltay = -deltay;
	
	if (x2 >= x1) 
	{             					// The x-values are increasing
	  xinc1 = 1;
	  xinc2 = 1;
    } 
	else 
	{          	         			// The x-values are decreasing
	  xinc1 = -1;
	  xinc2 = -1;
	}
	
	if (y2 >= y1)       	      	// The y-values are increasing
	{
	  yinc1 = 1;
	  yinc2 = 1;
	}
	else                    	  	// The y-values are decreasing
	{
	  yinc1 = -1;
	  yinc2 = -1;
	}
	
	if (deltax >= deltay)     		// There is at least one x-value for every y-value
	{
	  xinc1 = 0;              		// Don't change the x when numerator >= denominator
	  yinc2 = 0;              		// Don't change the y for every iteration
	  den = deltax;
	  num = deltax / 2;
	  numadd = deltay;
	  numpixels = deltax;     		// There are more x-values than y-values
	}
	else                      		// There is at least one y-value for every x-value
	{
	  xinc2 = 0;              		// Don't change the x for every iteration
	  yinc1 = 0;              		// Don't change the y when numerator >= denominator
	  den = deltay;
	  num = deltay / 2;
	  numadd = deltax;
	  numpixels = deltay;     		// There are more y-values than x-values
	}
	
	for (curpixel = 0; curpixel <= numpixels; curpixel++)
	{
	  PutPixel(x, y, colour);    	// Draw the current pixel
	  num += numadd;          		// Increase the numerator by the top of the fraction
	  if (num >= den)         		// Check if numerator >= denominator
	  {
		num -= den;           		// Calculate the new numerator value
		x += xinc1;           		// Change the x as appropriate
		y += yinc1;           		// Change the y as appropriate
	  }
	  x += xinc2;             		// Change the x as appropriate
	  y += yinc2;             		// Change the y as appropriate
	}
}

/*********************************************************************************
  * @brief  A box
  * @param  None
  * @retval None
*********************************************************************************/
void LCD_DrawRectangle(int x1, int y1, int x2, int y2, char colour)
{
	// Top
	LCD_DrawLine(x1, y1, x2, y1, colour);
	// Left
	LCD_DrawLine(x1, y1, x1, y2, colour);
	// Bottom
	LCD_DrawLine(x1, y2, x2, y2, colour);
	// Right
	LCD_DrawLine(x2, y1, x2, y2, colour);
}
/*********************************************************************************
  * @brief  A filled box
  * @param  None
  * @retval None
*********************************************************************************/
void LCD_DrawFilledRectangle(int x1, int y1, int x2, int y2, char colour)
{
	for(; y1<=y2; y1++)
		LCD_DrawLineX(x1, x2, y1, colour);
}

/*********************************************************************************
  * @brief  A rounded box
  * @param  None
  * @retval None
*********************************************************************************/
void LCD_DrawRoundedRectangle(int x1, int y1, int x2, int y2, char colour)
{
	// Top
	LCD_DrawLine(x1 + 1, y1, x2 - 1, y1, colour);
	// Left
	LCD_DrawLine(x1, y1 + 1, x1, y2 - 1, colour);
	// Bottom
	LCD_DrawLine(x1 + 1, y2, x2 - 1, y2, colour);
	// Right
	LCD_DrawLine(x2, y1 + 1, x2, y2 - 1, colour);
}
/*********************************************************************************
  * @brief  
  * @param  None
  * @retval None
*********************************************************************************/
void LCD_DrawCircle(unsigned char centre_x, unsigned char centre_y, unsigned char radius, unsigned char colour)
{	
	int xc = 0, yc, p;
	yc=radius;
	p =3 - (radius<<1);

    while (xc <= yc)
    {
      PutPixel (centre_x + xc, centre_y + yc, colour);
      PutPixel (centre_x + xc, centre_y - yc, colour);
      PutPixel (centre_x - xc, centre_y + yc, colour);
      PutPixel (centre_x - xc, centre_y - yc, colour);
      PutPixel (centre_x + yc, centre_y + xc, colour);
      PutPixel (centre_x + yc, centre_y - xc, colour);
      PutPixel (centre_x - yc, centre_y + xc, colour);
      PutPixel (centre_x - yc, centre_y - xc, colour);
		
      if (p < 0)
        p += (xc++ << 2) + 6;
      else
        p += ((xc++ - yc--)<<2) + 10;
    }
}
/*********************************************************************************
  * @brief  
  * @param  None
  * @retval None
*********************************************************************************/
void LCD_DrawFilledCircle(unsigned char centre_x, unsigned char centre_y, unsigned char radius, unsigned char colour)
{
	
	int xc = 0, yc, p;
	yc=radius;
	p =3 - (radius<<1);

    while (xc <= yc)
    {
		LCD_DrawLine (centre_x, centre_y, centre_x + xc, centre_y + yc, colour);
		LCD_DrawLine (centre_x, centre_y, centre_x + xc, centre_y - yc, colour);
		LCD_DrawLine (centre_x, centre_y, centre_x - xc, centre_y + yc, colour);
		LCD_DrawLine (centre_x, centre_y, centre_x - xc, centre_y - yc, colour);
		LCD_DrawLine (centre_x, centre_y, centre_x + yc, centre_y + xc, colour);
		LCD_DrawLine (centre_x, centre_y, centre_x + yc, centre_y - xc, colour);
		LCD_DrawLine (centre_x, centre_y, centre_x - yc, centre_y + xc, colour);
		LCD_DrawLine (centre_x, centre_y, centre_x - yc, centre_y - xc, colour);

		if (p < 0)
			p += (xc++ << 2) + 6;
		else
			p += ((xc++ - yc--)<<2) + 10;
    }
}


/*********************************************************************************
  * @brief  Линия, паралельная оси X
  * @param  None
  * @retval None
*********************************************************************************/
void LCD_DrawLineX(int x1, int x2, int y, char colour)
{
	do
	{
		PutPixel(x1, y, colour);
		x1++;
	}
	while (x1!=x2);
	PutPixel(x1,y,colour);
}

/*********************************************************************************
  * @brief  Линия, паралельная оси Y
  * @param  None
  * @retval None
*********************************************************************************/
void LCD_DrawLineY(int x, int y1, int y2, char colour)
{
	do
	{
		PutPixel(x, y1, colour);
		y1++;
	}
	while (y1!=y2);
	PutPixel(x,y1,colour);
}















