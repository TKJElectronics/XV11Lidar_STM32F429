#include "stm32f4xx.h"
#include "Serial.h"
#include "math.h"
#include "string.h"

#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ioe.h"

#define XV11_PACKAGE_LENGTH	22
#define XV11_START_BYTE	0xFA

uint8_t XV11_Package[22];

uint16_t Distance[360];
uint32_t PrintTimestamp;
uint32_t DisplayTimestamp;

uint16_t PackageChecksum(uint8_t * packagePointer);

static void LCD_Config(void);
static void LCD_AF_GPIOConfig(void);
static void delay(__IO uint32_t nCount);

const float RAD = (M_TWOPI / 360.0f);
uint16_t deg;
float radians, displayDistance;
uint16_t x[360], y[360];
uint16_t degSelected[2];
uint8_t scanEnabled = 1;
uint8_t pointSelect = 0;
float sqrtDist;
float tempCalc, tempCalc2, tempCalc3;

uint8_t textBuffer[30];

static TP_STATE* TP_State;
static void TP_Config(void);

int main(void)
{
	RCC_ClocksTypeDef RCC_Clocks;

	/* SysTick end of count event each 10us */
	RCC_GetClocksFreq(&RCC_Clocks);
	SysTick_Config(RCC_Clocks.SYSCLK_Frequency / 1000);

    /* Initialize the LCD */
    LCD_Init();
    LCD_LayerInit();

    /* Set LCD background layer */
    LCD_SetLayer(LCD_BACKGROUND_LAYER);

    /* Set LCD transparency */
    LCD_SetTransparency(0);

    /* Set LCD foreground layer */
    LCD_SetLayer(LCD_FOREGROUND_LAYER);

    /* LTDC reload configuration */
    LTDC_ReloadConfig(LTDC_IMReload);

    /* Enable the LTDC */
    LTDC_Cmd(ENABLE);


    /* Touch Panel configuration */
    TP_Config();

    LCD_Clear(LCD_COLOR_BLACK);

    LCD_SetTextColor(LCD_COLOR_BLUE);
    LCD_DrawFullRect(0, 253, 240, 40);
    LCD_SetFont(&Font16x24);
    LCD_SetTextColor(LCD_COLOR_WHITE);
    LCD_SetBackColor(LCD_COLOR_BLUE);
    LCD_DisplayStringLine(LINE(11), (uint8_t*)"  PAUSE SCAN  ");

	Serial_Init();
	SyncUp();

	for (deg = 0; deg < 360; deg++) { // reset pixels
		x[deg] = 120;
		y[deg] = 120;
	}

    while(1)
    {
    	if (scanEnabled == 1) {
    		while (Serial_Buffer_Count() < XV11_PACKAGE_LENGTH);
    		LoadPackage(XV11_Package);
    		if (XV11_Package[0] != XV11_START_BYTE) {
    			SyncUp();
    		} else {
    			ParsePackage(XV11_Package);
    		}

    		if (Millis() > PrintTimestamp) {
    			PrintTimestamp = Millis() + 200;  // print every 200 ms
    			Serial_WriteByte(0xAA);
    			Serial_WriteByte(0xBB);
    			Serial_WriteByte(0xCC);
    			Serial_WriteWords(Distance, 360);
    			Serial_WriteByte(0xCC);
    			Serial_WriteByte(0xBB);
    			Serial_WriteByte(0xAA);
    		}

    		if (Millis() > DisplayTimestamp) {
    			DisplayTimestamp = Millis() + 50;  // print every 50 ms
    			DrawDistanceMap();

    	        TP_State = IOE_TP_GetState();
    	        if (TP_State->TouchDetected) {
    				Delay_Ms(50);
    				TP_State = IOE_TP_GetState();
    				if (!TP_State->TouchDetected) continue;

   	        		if ((TP_State->X >= 0 && TP_State->X <= 240) && (TP_State->Y >= 253 && TP_State->Y <= 293))
   	        		{
    	        		while (TP_State->TouchDetected) {
    	        			TP_State = IOE_TP_GetState();
    	        		}
    	        		scanEnabled = 0;
    	            	LCD_SetTextColor(LCD_COLOR_BLUE);
    	            	LCD_DrawFullRect(0, 253, 240, 40);
    	            	LCD_SetFont(&Font16x24);
    	            	LCD_SetTextColor(LCD_COLOR_WHITE);
    	            	LCD_SetBackColor(LCD_COLOR_BLUE);
    	            	LCD_DisplayStringLine(LINE(11), (uint8_t*)"  RESUME SCAN ");
    	        	}
    	        }
    		}
    	}
    	else // scan not enabled
    	{
    		Serial_Buffer_Clear(); // keep discarding the serial input

    		TP_State = IOE_TP_GetState();
    		if (TP_State->TouchDetected) {
				Delay_Ms(50);
				TP_State = IOE_TP_GetState();
				if (!TP_State->TouchDetected) continue;

    			if ((TP_State->X >= 0 && TP_State->X <= 240) && (TP_State->Y >= 253 && TP_State->Y <= 293))
    			{
    				while (TP_State->TouchDetected) {
    					TP_State = IOE_TP_GetState();
    				}
    				scanEnabled = 1;
    	            LCD_Clear(LCD_COLOR_BLACK);
    				LCD_SetTextColor(LCD_COLOR_BLUE);
    				LCD_DrawFullRect(0, 253, 240, 40);
    				LCD_SetFont(&Font16x24);
    				LCD_SetTextColor(LCD_COLOR_WHITE);
    				LCD_SetBackColor(LCD_COLOR_BLUE);
    				LCD_DisplayStringLine(LINE(11), (uint8_t*)"  PAUSE SCAN  ");
    				SyncUp();
    			} else if ((TP_State->X >= 0 && TP_State->X <= 240) && (TP_State->Y >= 0 && TP_State->Y <= 240)) {
    				Delay_Ms(50);
    				TP_State = IOE_TP_GetState();
    				if (!TP_State->TouchDetected) continue;

    				if (pointSelect < 2) {
    					sqrtDist = 1000; // max distance
    					degSelected[pointSelect] = 0xFFFF;
    					for (deg = 0; deg < 360; deg++) {
    						tempCalc = x[deg] - TP_State->X;
    						tempCalc2 = y[deg] - TP_State->Y;
    						tempCalc = tempCalc * tempCalc;
    						tempCalc2 = tempCalc2 * tempCalc2;
    						tempCalc3 = sqrt(tempCalc + tempCalc2);
    						if (tempCalc3 < sqrtDist) {
    							sqrtDist = tempCalc3;
    							degSelected[pointSelect] = deg;
    						}
    					}
    					if (degSelected[pointSelect] != 0xFFFF) {
    						LCD_SetTextColor(LCD_COLOR_RED);
    						LCD_DrawFullCircle(x[degSelected[pointSelect]], y[degSelected[pointSelect]], 6);
    						pointSelect++;
    					}

    					if (pointSelect == 2) {
    						// do calculation
    						LCD_SetTextColor(LCD_COLOR_GREEN);
    						LCD_DrawUniLine(x[degSelected[0]], y[degSelected[0]], x[degSelected[1]], y[degSelected[1]]);
    						tempCalc = x[degSelected[0]] - x[degSelected[1]];
    						tempCalc2 = y[degSelected[0]] - y[degSelected[1]];
    						tempCalc = tempCalc * tempCalc;
    						tempCalc2 = tempCalc2 * tempCalc2;
    						tempCalc3 = sqrt(tempCalc + tempCalc2);
    						tempCalc3 *= 25.0f; // convert to mm
    						tempCalc3 /= 10; // convert to cm
    						sprintf(textBuffer, "    %d cm ", (uint16_t)tempCalc3);
    						LCD_SetTextColor(LCD_COLOR_WHITE);
    						LCD_SetBackColor(LCD_COLOR_BLACK);
    						LCD_DisplayStringLine(LINE(9), textBuffer);
    					}
    				} else {
    					LCD_Clear(LCD_COLOR_BLACK);
    					DrawDistanceMap();
    					LCD_SetTextColor(LCD_COLOR_BLUE);
    					LCD_DrawFullRect(0, 253, 240, 40);
    					LCD_SetFont(&Font16x24);
    					LCD_SetTextColor(LCD_COLOR_WHITE);
    					LCD_SetBackColor(LCD_COLOR_BLUE);
    					LCD_DisplayStringLine(LINE(11), (uint8_t*)"  RESUME SCAN ");
    					pointSelect = 0;
    				}

    				while (TP_State->TouchDetected) {
    					TP_State = IOE_TP_GetState();
    				}
    			}
    		}
    	}
    }
}


void SyncUp(void)
{
	uint8_t i;
	int16_t ch = 0;
	int16_t ch2 = 0;

	Serial_Buffer_Clear();

	while (ch != XV11_START_BYTE) {
		while (Serial_Buffer_Get() != XV11_START_BYTE);

		i = XV11_PACKAGE_LENGTH;
		while (i > 0) {
			ch = Serial_Buffer_Get();
			if (ch >= 0) i--;
		}

		// read the rest
		i = XV11_PACKAGE_LENGTH - 1;
		while (i > 0) {
			ch2 = Serial_Buffer_Get();
			if (ch2 >= 0) i--;
		}
	}
}

void LoadPackage(uint8_t * packagePointer)
{
	uint8_t i = 0;
	uint8_t ch;
	int16_t temp;

	while (i < XV11_PACKAGE_LENGTH) {
		temp = Serial_Buffer_Get();
		if (temp >= 0) {
			packagePointer[i] = temp;
			i++;
		} else {
			temp = 0;
		}
	}
}

uint16_t GoodReadings = 0, BadReadings = 0;
uint16_t AnglesCovered = 0;
void ParsePackage(uint8_t * packagePointer)
{
	uint16_t i;
	uint16_t Index;
	uint8_t Speed;
	uint8_t InvalidFlag[4];
	uint8_t WarningFlag[4];
	uint16_t Checksum, ChecksumCalculated;

	Checksum = ((uint16_t)packagePointer[21] << 8) | packagePointer[20];
	ChecksumCalculated = PackageChecksum(packagePointer);
	if (Checksum != ChecksumCalculated) {
		BadReadings += 4;
	}

	Index = (packagePointer[1] - 0xA0) * 4;
	Speed = ((uint16_t)packagePointer[3] << 8) | packagePointer[2];
	InvalidFlag[0] = (packagePointer[5] & 0x80) >> 7;
	InvalidFlag[1] = (packagePointer[9] & 0x80) >> 7;
	InvalidFlag[2] = (packagePointer[13] & 0x80) >> 7;
	InvalidFlag[3] = (packagePointer[17] & 0x80) >> 7;
	WarningFlag[0] = (packagePointer[5] & 0x40) >> 6;
	WarningFlag[1] = (packagePointer[9] & 0x40) >> 6;
	WarningFlag[2] = (packagePointer[13] & 0x40) >> 6;
	WarningFlag[3] = (packagePointer[17] & 0x40) >> 6;

	if (Index == 0) {
		AnglesCovered = 0;
		for (i = 0; i < 360; i++) {
			if (Distance[i] > 0) AnglesCovered++;
		}

		GoodReadings = 0;
		BadReadings = 0;
	}

	for (i = 0; i < 4; i++) {
		if (!InvalidFlag[i])
		{
			Distance[Index+i] = packagePointer[4+(i*4)] | ((uint16_t)(packagePointer[5+(i*4)] & 0x3F) << 8);
			GoodReadings++;
		} else {
			Distance[Index+i] = 0;
			BadReadings++;
		}
	}

}

uint16_t PackageChecksum(uint8_t * packagePointer)
{
	uint8_t i;
	uint16_t data[10];
	uint16_t checksum;
	uint32_t chk32;

	// group the data by word, little-endian
	for (i = 0; i < 10; i++) {
		data[i] = packagePointer[2*i] | (((uint16_t)packagePointer[2*i+1]) << 8);
	}

	// compute the checksum on 32 bits
	chk32 = 0;
	for (i = 0; i < 10; i++) {
    	chk32 = (chk32 << 1) + data[i];
	}

   // return a value wrapped around on 15bits, and truncated to still fit into 15 bits
   checksum = (chk32 & 0x7FFF) + ( chk32 >> 15 ); // wrap around to fit into 15 bits
   checksum = checksum & 0x7FFF; // truncate to 15 bits
}


void DrawDistanceMap(void)
{
	// UP TO 3500 i distance = 3.5m
	radians = 0;
	for (deg = 0; deg < 360; deg++) {
		LCD_SetTextColor(LCD_COLOR_BLACK);
		LCD_DrawLine(x[deg], y[deg], 1, LCD_DIR_HORIZONTAL); // PutPixel(x, y);
		LCD_DrawLine(x[deg]+1, y[deg], 1, LCD_DIR_HORIZONTAL); // PutPixel(x, y);
		LCD_DrawLine(x[deg], y[deg]+1, 1, LCD_DIR_HORIZONTAL); // PutPixel(x, y);
		LCD_DrawLine(x[deg]+1, y[deg]+1, 1, LCD_DIR_HORIZONTAL); // PutPixel(x, y);

		if (Distance[deg] < 3000 && Distance[deg] > 0) {
			displayDistance = Distance[deg] / 25.0f;
			LCD_SetTextColor(LCD_COLOR_WHITE);
			x[deg] = (cos(M_PI_2-radians) * displayDistance) + 120;
			y[deg] = (sin(M_PI_2-radians) * displayDistance) + 120;
			LCD_DrawLine(x[deg], y[deg], 1, LCD_DIR_HORIZONTAL); // PutPixel(x, y);
			LCD_DrawLine(x[deg]+1, y[deg], 1, LCD_DIR_HORIZONTAL); // PutPixel(x, y);
			LCD_DrawLine(x[deg], y[deg]+1, 1, LCD_DIR_HORIZONTAL); // PutPixel(x, y);
			LCD_DrawLine(x[deg]+1, y[deg]+1, 1, LCD_DIR_HORIZONTAL); // PutPixel(x, y);
		}
		radians += RAD;
	}
	LCD_SetTextColor(LCD_COLOR_BLUE2);
	LCD_DrawFullCircle(120, 120, 6);
}

/**
* @brief  Configure the IO Expander and the Touch Panel.
* @param  None
* @retval None
*/
static void TP_Config(void)
{
  /* Clear the LCD */
  LCD_Clear(LCD_COLOR_WHITE);

  /* Configure the IO Expander */
  if (IOE_Config() == IOE_OK)
  {
    LCD_SetFont(&Font8x8);
    LCD_DisplayStringLine(LINE(32), (uint8_t*)"              Touch Panel Paint     ");
    LCD_DisplayStringLine(LINE(34), (uint8_t*)"              Example               ");
    LCD_SetTextColor(LCD_COLOR_BLUE2);
    LCD_DrawFullRect(5, 250, 30, 30);
    LCD_SetTextColor(LCD_COLOR_CYAN);
    LCD_DrawFullRect(40, 250, 30, 30);
    LCD_SetTextColor(LCD_COLOR_YELLOW);
    LCD_DrawFullRect(75, 250, 30, 30);
    LCD_SetTextColor(LCD_COLOR_RED);
    LCD_DrawFullRect(5, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_BLUE);
    LCD_DrawFullRect(40, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_GREEN);
    LCD_DrawFullRect(75, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_MAGENTA);
    LCD_DrawFullRect(145, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_BLACK);
    LCD_DrawFullRect(110, 288, 30, 30);
    LCD_DrawRect(180, 270, 48, 50);
    LCD_SetFont(&Font16x24);
    LCD_DisplayChar(LCD_LINE_12, 195, 0x43);
    LCD_DrawLine(0, 248, 240, LCD_DIR_HORIZONTAL);
    LCD_DrawLine(0, 284, 180, LCD_DIR_HORIZONTAL);
    LCD_DrawLine(1, 248, 71, LCD_DIR_VERTICAL);
    LCD_DrawLine(37, 248, 71, LCD_DIR_VERTICAL);
    LCD_DrawLine(72, 248, 71, LCD_DIR_VERTICAL);
    LCD_DrawLine(107, 248, 71, LCD_DIR_VERTICAL);
    LCD_DrawLine(142, 284, 36, LCD_DIR_VERTICAL);
    LCD_DrawLine(0, 319, 240, LCD_DIR_HORIZONTAL);
  }
  else
  {
    LCD_Clear(LCD_COLOR_RED);
    LCD_SetTextColor(LCD_COLOR_BLACK);
    LCD_DisplayStringLine(LCD_LINE_6,(uint8_t*)"   IOE NOT OK      ");
    LCD_DisplayStringLine(LCD_LINE_7,(uint8_t*)"Reset the board   ");
    LCD_DisplayStringLine(LCD_LINE_8,(uint8_t*)"and try again     ");
  }
}

