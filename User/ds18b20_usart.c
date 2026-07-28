/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ds18b20.c
  * @brief          : DS18B20 driver implementation
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes --------------------------------------------------------------------*/
#include "ds18b20_usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "led_module.h"
#include "main.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define OW_BAUD_RESET    9600U
#define OW_BAUD_DATA     57600  /* was 115200 -> 57600 -> 38400. Dropped the
                                      filler-byte padding trick (it only made
                                      our own loopback self-consistent, not
                                      necessarily correct from the external
                                      device's independent sampling clock).
                                      Simple plain bursts, just slower overall,
                                      to give real settling margin per bit.
                                      Write-0 slot at this baud: 9 bit periods
                                      / 28800 ≈ 312us — safely under the
                                      ~480us reset-detection threshold. */
#define OW_DMA_TIMEOUT   15U      /* ms, one byte at 28800 baud takes ~347us  */

#define DS18B20_CMD_SEARCH_ROM     0xF0U
#define DS18B20_CMD_MATCH_ROM      0x55U
#define DS18B20_CMD_SKIP_ROM       0xCCU
#define DS18B20_CMD_CONVERT_T      0x44U
#define DS18B20_CMD_WRITE_SCRATCH  0x4EU
#define DS18B20_CMD_READ_SCRATCH   0xBEU
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
DS18B20_t	DS18B20;

uint8_t sensorCount;
int16_t testTemp1;
int16_t testTemp1Old;
int16_t testTemp2;
int16_t testTemp2Old;
//static UART_HandleTypeDef *ow_huart = NULL;

/* DMA completion flags, set from HAL_UART_RxCpltCallback / TxCpltCallback */
volatile uint8_t owRxDone = 0;
volatile uint8_t owTxDone = 0;

/* USER CODE END PV */

/* Private function prototypes ------------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code -----------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  Reconfigure USART baud rate by writing BRR directly.
  * @note   Deliberately does NOT call HAL_UART_DeInit()/HAL_UART_Init():
  *         those reset the CR3 HDSEL (half-duplex) bit configured by
  *         MX_USART1_UART_Init(), which breaks single-wire echo capture.
  *         Assumes OverSampling = 16, ClockPrescaler = DIV1 (as generated
  *         by CubeMX) and that ow_huart->Instance clock source is PCLK1.
  *         Must not be called while a DMA transfer is in progress.
  */
static void OW_SetBaud(uint32_t baud)
{
	uint32_t pclk = LL_RCC_GetUSARTClockFreq(LL_RCC_USART1_CLKSOURCE);
	uint32_t prescaler = LL_USART_GetPrescaler(USART1);
	LL_USART_Disable(USART1);
	LL_USART_SetBaudRate(USART1, pclk, prescaler, LL_USART_OVERSAMPLING_16, baud);
	LL_USART_Enable(USART1);
	while (!LL_USART_IsActiveFlag_TEACK(USART1)) { }
	while (!LL_USART_IsActiveFlag_REACK(USART1)) { }
}

/**
  * @brief  Wait for BOTH DMA TX and RX completion flags, with a timeout.
  * @note   Waiting on RX alone is not enough: gState (TX) and RxState (RX)
  *         are independent HAL state machines and TX can flip back to
  *         READY a few cycles after RX does. Starting the next
  *         HAL_UART_Transmit_DMA() before that happens returns HAL_BUSY,
  *         which OW_Transceive() would otherwise misreport as a bus error.
  *         __WFI() lets the core sleep between interrupts while waiting
  *         instead of pure busy-loop spinning.
  */
static HAL_StatusTypeDef OW_WaitRx(uint32_t timeoutMs)
{
    uint32_t start = HAL_GetTick();

    while (!(owRxDone && owTxDone))
    {
        if ((HAL_GetTick() - start) > timeoutMs)
        {
        	LL_USART_DisableDMAReq_RX(USART1);
			LL_USART_DisableDMAReq_TX(USART1);
			LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_3);
			LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
            owRxDone = 0;
            owTxDone = 0;
            return HAL_TIMEOUT;
        }
        __WFI();
    }
    owRxDone = 0;
    owTxDone = 0;
    return HAL_OK;
}

/**
  * @brief  Transmit len bytes and capture the hardware-looped-back echo
  *         (single-wire mode) into rx[], via DMA.
  * @note   RX DMA is armed BEFORE TX DMA is started, so every echoed bit
  *         is captured as it is clocked out.
  */
static HAL_StatusTypeDef OW_Transceive(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
	LL_DMA_ClearFlag_TC3(DMA1); // Очищаем флаг окончания предыдущего приема (Канал 3)
	LL_DMA_ClearFlag_TE3(DMA1); // Очищаем флаг ошибки приема
	LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_3);

	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t)rx);
	LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, len);

	// Включаем канал приема первого, чтобы он был готов слушать шину до начала передачи
	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);
	LL_USART_EnableDMAReq_RX(USART1); // Разрешаем USART1 просить DMA забирать данные

	// --- Шаг 2. Конфигурация DMA для ПЕРЕДАЧИ (TX) ---
	LL_DMA_ClearFlag_TC2(DMA1); // Очищаем флаг окончания предыдущей передачи (Канал 2)
	LL_DMA_ClearFlag_TE2(DMA1); // Очищаем флаг ошибки передачи
	LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);

	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_4, (uint32_t)tx);
	LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_4, len);

	// Включаем канал передачи и пинаем USART1 на отправку запросов
	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);
	LL_USART_EnableDMAReq_TX(USART1);
    return OW_WaitRx(OW_DMA_TIMEOUT);
}

/**
  * @brief  1-Wire reset + presence detect.
  * @retval 1 = at least one device answered, 0 = no presence pulse
  */
static uint8_t OW_Reset(void)
{
//  uint8_t tx = 0xF0, rx = 0;		// tx = 0xF0 - , 0xA0 +
    uint8_t tx = 0xA0, rx = 0;		// Why A0 works?

    OW_SetBaud(OW_BAUD_RESET);

	if (OW_Transceive(&tx, &rx, 1) != HAL_OK)
	{
		OW_SetBaud(OW_BAUD_DATA);
		return 0;
	}
	OW_SetBaud(OW_BAUD_DATA);
	if(rx != 0xF0)
		return 1;
	else
		return 0;
}

/**
  * @brief  Single 1-Wire bit exchange. Send txBit (1 = release/read slot),
  *         return the bit sampled back.
  */
static uint8_t OW_Bit(uint8_t txBit)
{
    uint8_t tx = txBit ? 0xFF : 0x00;
    uint8_t rx = 0;

    if ( OW_Transceive(&tx, &rx, 1))
    {
        return 1; /* fail-safe: report "1" (idle line) on timeout */
    }
    return (rx == 0xFF) ? 1 : 0;
}

/**
  * @brief  Full byte exchange as a single 8-byte DMA burst (one UART byte
  *         per 1-Wire bit). For reading, pass 0xFF as txByte and read the
  *         returned value.
  */

/**
  * @brief  Full byte exchange as a single plain DMA burst (one UART byte
  *         per 1-Wire bit, no filler/padding tricks). Used for both writes
  *         and reads — a filler-based scheme was tried and dropped: it made
  *         our own loopback echo self-consistent, but that only proves our
  *         own UART sampled what it itself sent, not that the external
  *         device (sampling independently, on its own clock) saw the same
  *         thing. Simpler and safer to just slow the whole baud down
  *         instead — see OW_BAUD_DATA.
  */
static uint8_t OW_ByteIO(uint8_t txByte)
{
    uint8_t txBuf[8], rxBuf[8];
    uint8_t result = 0;

    for (uint8_t i = 0; i < 8; i++)
    {
        txBuf[i] = (txByte & (1U << i)) ? 0xFF : 0x00;
    }

    if (OW_Transceive(txBuf, rxBuf, 8))
    {
        return 0xFF;
    }

    for (uint8_t i = 0; i < 8; i++)
    {
        if (rxBuf[i] == 0xFF)
        {
            result |= (1U << i);
        }
    }
    return result;
}

/**
  * @brief  Dallas/Maxim CRC-8 (poly 0x31, reflected), used to validate
  *         ROM codes and scratchpad contents.
  */
static uint8_t OW_CRC8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;

    for (uint8_t i = 0; i < len; i++)
    {
        uint8_t inByte = data[i];
        for (uint8_t b = 0; b < 8; b++)
        {
            uint8_t mix = (crc ^ inByte) & 0x01;
            crc >>= 1;
            if (mix)
            {
                crc ^= 0x8C;
            }
            inByte >>= 1;
        }
    }
    return crc;
}

/**
  * @brief  One iteration of the standard Maxim AN187 1-Wire ROM search.
  *         Finds the next device on the bus and stores its ROM in romNo[].
  * @retval 1 = a device ROM was found and written to romNo[], 0 = search done
  */
static uint8_t OW_SearchROM(void)
{
    uint8_t idBitNumber = 1;
    uint8_t lastZero = 0;
    uint8_t romByteNumber = 0;
    uint8_t romByteMask = 1;
    uint8_t searchResult = 0;
    uint8_t idBit, cmpIdBit, searchDirection;

    if (DS18B20.lastDeviceFlag)
    {
        return 0;
    }

    if (!OW_Reset())
    {
    	DS18B20.lastDiscrepancy = 0;
    	DS18B20.lastDeviceFlag = 0;
    	DS18B20.lastFamilyDiscrepancy = 0;
    	return 0;
    }

    OW_ByteIO(DS18B20_CMD_SEARCH_ROM);

    do
    {
        idBit    = OW_Bit(1);
        cmpIdBit = OW_Bit(1);

        if (idBit && cmpIdBit)
        {
            break; /* no devices responded */
        }

        if (idBit != cmpIdBit)
        {
            searchDirection = idBit;
        }
        else
        {
            if (idBitNumber < DS18B20.lastDiscrepancy)
            {
                searchDirection = (DS18B20.romNo[romByteNumber] & romByteMask) ? 1 : 0;
            }
            else
            {
                searchDirection = (idBitNumber == DS18B20.lastDiscrepancy) ? 1 : 0;
            }
            if (searchDirection == 0)
            {
                lastZero = idBitNumber;
                if (lastZero < 9)
                {
                	DS18B20.lastFamilyDiscrepancy = lastZero;
                }
            }
        }

        if (searchDirection)
        {
        	DS18B20.romNo[romByteNumber] |= romByteMask;
        }
        else
        {
        	DS18B20.romNo[romByteNumber] &= (uint8_t)~romByteMask;
        }

        OW_Bit(searchDirection);

        idBitNumber++;
        romByteMask <<= 1;
        if (romByteMask == 0)
        {
            romByteNumber++;
            romByteMask = 1;
        }
    } while (romByteNumber < 8);

    if (idBitNumber >= 65)
    {
    	DS18B20.lastDiscrepancy = lastZero;
        if (DS18B20.lastDiscrepancy == 0)
        {
        	DS18B20.lastDeviceFlag = 1;
        }
        searchResult = 1;
    }

    if (!searchResult || (OW_CRC8(DS18B20.romNo, 7) != DS18B20.romNo[7]))
    {
    	DS18B20.lastDiscrepancy = 0;
    	DS18B20.lastDeviceFlag = 0;
    	DS18B20.lastFamilyDiscrepancy = 0;
        searchResult = 0;
    }

    return searchResult;
}

/**
  * @brief  Match ROM + Read Scratchpad for one already-known sensor.
  *         Called only from DS18B20_Process() once conversion time elapsed.
  */
static void DS18B20_ReadOne(uint8_t index)
{
    uint8_t scratch[9];

    if (!OW_Reset())
    {
    	DS18B20.sensors[index].valid = 0;
        return;
    }

    OW_ByteIO(DS18B20_CMD_MATCH_ROM);
    for (uint8_t i = 0; i < 8; i++)
    {
        OW_ByteIO(DS18B20.sensors[index].rom[i]);
    }
    OW_ByteIO(DS18B20_CMD_READ_SCRATCH);

    for (uint8_t i = 0; i < 9; i++)
    {
        scratch[i] = OW_ByteIO(0xFF);
    }

    if (OW_CRC8(scratch, 8) != scratch[8])
    {
    	DS18B20.sensors[index].valid = 0;
        return;
    }

    int16_t raw = (int16_t)((scratch[1] << 8) | scratch[0]);
    DS18B20.sensors[index].tempRaw = raw;
    DS18B20.sensors[index].valid = 1;
}

/* USER CODE END 0 */

/* Exported functions ----------------------------------------------------------*/
/* USER CODE BEGIN EF */

/**
  * @brief  Store the USART handle to use. Call once after MX_USART2_UART_Init().
  * @note   huart must already be configured for Single Wire (half-duplex) mode.
  */
void DS18B20_Init()
{
    sensorCount = 0;
    OW_SetBaud(OW_BAUD_DATA);
    memset((void*)&DS18B20, 0, sizeof(DS18B20_t));
}

/**
  * @brief  Enumerate every DS18B20 on the bus via ROM search.
  *         Blocking (runs at startup, not time-critical). Fills the
  *         internal sensor table.
  * @retval Number of sensors found (0..DS18B20_MAX_SENSORS)
  */
uint8_t DS18B20_SearchAll(void)
{
    sensorCount = 0;
    DS18B20.lastDiscrepancy = 0;
    DS18B20.lastDeviceFlag = 0;
    DS18B20.lastFamilyDiscrepancy = 0;

    while (OW_SearchROM() && (sensorCount < DS18B20_MAX_SENSORS))
    {
        memcpy(DS18B20.sensors[sensorCount].rom, DS18B20.romNo, 8);
        DS18B20.sensors[sensorCount].valid = 0;
        DS18B20.sensors[sensorCount].tempRaw = 0;
        sensorCount++;
    }

    return sensorCount;
}

/**
  * @brief  Non-blocking state machine.
  * - 1 sensor
  * - Skip ROM applied
  * - debug registers left
  */
void DS18B20_Processing1()
{
	uint8_t scratch[9];

	switch(DS18B20.MainStateMachine)
	{
		case 0:
//			MX_USART1_UART_Init();
//			DS18B20_Init(&huart1);
			DS18B20.Counter += 200;
			DS18B20.MainStateMachine++;
		break;

		case 1:
			if(DS18B20.Counter < SystemCounter) {
				DS18B20.Counter += 200;
				DS18B20.MainStateMachine++;
			}
		break;

		case 2:
			if(DS18B20.Counter < SystemCounter)
			{
				if (!OW_Reset())
			    {
					DS18B20.MainStateMachine = 0;
			        return;
			    }
				DS18B20.MainStateMachine++;
			}
		break;

		case 3:
			OW_ByteIO(DS18B20_CMD_SKIP_ROM);
			OW_ByteIO(DS18B20_CMD_CONVERT_T);
			DS18B20.Counter += 800;
			DS18B20.MainStateMachine++;
			break;

		case 4:
			if(DS18B20.Counter < SystemCounter)
			{
				if (!OW_Reset())
				{
					DS18B20.MainStateMachine = 1;
					return; /* lost presence between convert and read */
				}
				OW_ByteIO(DS18B20_CMD_SKIP_ROM);
				OW_ByteIO(DS18B20_CMD_READ_SCRATCH);
				for (uint8_t i = 0; i < 9; i++)
				{
					scratch[i] = OW_ByteIO(0xFF);
				}
				if (OW_CRC8(scratch, 8) != scratch[8])
				{
					DS18B20.MainStateMachine = 1;
					return; /* CRC mismatch */
				}
				testTemp1 = (int16_t)((scratch[1] << 8) | scratch[0]);

				DS18B20.MainStateMachine++;
			}
			break;

		case 5:
			DS18B20.MainStateMachine = 1;
			break;

		default:
			DS18B20.MainStateMachine = 1;
			break;
	}
}



/**
  * @brief  Non-blocking state machine. Call from the main loop.
  *         Does nothing until DS18B20_CONV_TIME_MS has elapsed since
  *         DS18B20_StartConversion(), then reads every sensor's
  *         scratchpad (short synchronous burst, no HAL_Delay involved).
  */
void DS18B20_Processing()
{
	switch(DS18B20.MainStateMachine)
	{
		case 0:
//			MX_USART1_UART_Init();
//			DS18B20_Init(&huart1);
			if(DS18B20.Counter < SystemCounter)
			{
				for (uint8_t index = 0; index < sensorCount; index++)
				{
					if (!OW_Reset())
					{
						DS18B20.MainStateMachine = 1;
						return;
					}
					OW_ByteIO(DS18B20_CMD_MATCH_ROM);
					for (uint8_t i = 0; i < 8; i++)
					{
						OW_ByteIO(DS18B20.sensors[index].rom[i]);
					}
					OW_ByteIO(DS18B20_CMD_WRITE_SCRATCH);
					OW_ByteIO(0x4B);
					OW_ByteIO(0x46);
					OW_ByteIO(0x5F);		// Config 11 bit
				}

				DS18B20.Counter += 200;
				DS18B20.MainStateMachine++;
			}
		break;

		case 1:
			DS18B20.Counter += 200;
			DS18B20.MainStateMachine++;
			break;

		case 2:
			if(DS18B20.Counter < SystemCounter)
			{
				if (!OW_Reset())
			    {
					DS18B20.MainStateMachine = 1;
			        return;
			    }
				OW_ByteIO(DS18B20_CMD_SKIP_ROM);
				OW_ByteIO(DS18B20_CMD_CONVERT_T);
				DS18B20.Counter += 395;
				DS18B20.MainStateMachine++;
			}
		break;

		case 3:
			if(DS18B20.Counter < SystemCounter)
			{
				for (uint8_t i = 0; i < sensorCount; i++)
				{
					DS18B20_ReadOne(i);
				}
				testTemp1 = DS18B20.sensors[0].tempRaw;
				testTemp2 = DS18B20.sensors[1].tempRaw;
				DS18B20.MainStateMachine++;
			}
			break;

		case 4:
			DS18B20.MainStateMachine = 1;
			break;

		default:
			DS18B20.MainStateMachine = 1;
			break;
	}
}


/**
  * @brief  DIAGNOSTIC ONLY. Bypasses ROM search entirely — assumes exactly
  *         one sensor on the bus and talks to it via Skip ROM (0xCC).
  *         Blocking, includes HAL_Delay(750) on purpose (diagnostics, not
  *         production code). Remove once the search-based path is confirmed
  *         working, or keep as a manual single-sensor fallback.
  * @param  outTemp: pointer to receive the temperature on success
  * @retval 1 = success (CRC valid), 0 = reset/CRC failure
  */
uint8_t DS18B20_QuickTestSkipROM(int16_t *outRaw)
{
    uint8_t scratch[9];

    if (!OW_Reset()) {
        return 0; /* no presence at all */
    }

    OW_ByteIO(DS18B20_CMD_SKIP_ROM);
    OW_ByteIO(DS18B20_CMD_CONVERT_T);
    HAL_Delay(750); /* diagnostic only — blocking is fine here */

    if (!OW_Reset()) {
        return 0; /* lost presence between convert and read */
    }
    OW_ByteIO(DS18B20_CMD_SKIP_ROM);
    OW_ByteIO(DS18B20_CMD_READ_SCRATCH);

    for (uint8_t i = 0; i < 9; i++) {
        scratch[i] = OW_ByteIO(0xFF);
    }

    if (OW_CRC8(scratch, 8) != scratch[8]) {
        return 0; /* CRC mismatch */
    }

    *outRaw = (int16_t)((scratch[1] << 8) | scratch[0]);
    return 1;
}


/* USER CODE END EF */
