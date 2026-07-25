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
#include "usart.h"
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
#define OW_DMA_TIMEOUT   5U      /* ms, one byte at 28800 baud takes ~347us  */

#define DS18B20_CMD_SEARCH_ROM     0xF0U
#define DS18B20_CMD_MATCH_ROM      0x55U
#define DS18B20_CMD_SKIP_ROM       0xCCU
#define DS18B20_CMD_CONVERT_T      0x44U
#define DS18B20_CMD_READ_SCRATCH   0xBEU
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
DS18B20_t	DS18B20;

static UART_HandleTypeDef *ow_huart = NULL;

static DS18B20_Sensor sensors[DS18B20_MAX_SENSORS];
static uint8_t         sensorCount = 1;
static DS18B20_State   state = DS18B20_STATE_IDLE;
static uint32_t        convStartTick = 0;
static uint8_t          dataReady = 0;

/* ROM search algorithm state (Maxim AN187) */
static uint8_t romNo[8];
static int      lastDiscrepancy = 0;
static int      lastFamilyDiscrepancy = 0;
static uint8_t  lastDeviceFlag = 0;

/* DMA completion flags, set from HAL_UART_RxCpltCallback / TxCpltCallback */
static volatile uint8_t owRxDone = 0;
static volatile uint8_t owTxDone = 0;

/* DIAGNOSTIC ONLY — status of each OW_Transceive() call, safe to inspect at any pause */
static volatile HAL_StatusTypeDef dbgByteStatus[9];
static volatile uint8_t dbgByteIndex = 0;
static volatile HAL_StatusTypeDef dbgSkipRomStatus;
static volatile HAL_StatusTypeDef dbgReadScratchStatus;
static volatile uint8_t dbgSkipRomEcho;
static volatile uint8_t dbgReadScratchEcho;
static volatile HAL_StatusTypeDef dbgLastByteStatus;  /* set on every single OW_ByteIO() call */
static volatile HAL_StatusTypeDef dbgResetStatus;
static volatile uint8_t dbgResetRx;
/* USER CODE END PV */

/* Private function prototypes ------------------------------------------------*/
/* USER CODE BEGIN PFP */
static void            OW_SetBaud(uint32_t baud);
static HAL_StatusTypeDef OW_WaitRx(uint32_t timeoutMs);
static HAL_StatusTypeDef OW_Transceive(const uint8_t *tx, uint8_t *rx, uint16_t len);

static uint8_t OW_Reset(void);
static uint8_t OW_Bit(uint8_t txBit);
static uint8_t OW_ByteIO(uint8_t txByte);

static uint8_t OW_SearchROM(void);
static uint8_t OW_CRC8(const uint8_t *data, uint8_t len);

static void DS18B20_ReadOne(uint8_t index);
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
    uint32_t pclk = HAL_RCC_GetPCLK1Freq();

    ow_huart->Instance->CR1 &= ~USART_CR1_UE;               /* disable USART */
    ow_huart->Instance->BRR = (pclk + (baud / 2U)) / baud;  /* oversampling 16 */
    ow_huart->Instance->CR1 |= USART_CR1_UE;                /* re-enable      */

	/* Дождаться, пока TX и RX реально включатся, иначе первый байт
	после переключения baud может быть потерян/испорчен. */
	while (!(ow_huart->Instance->ISR & USART_ISR_TEACK)) { }
	while (!(ow_huart->Instance->ISR & USART_ISR_REACK)) { }

	ow_huart->Init.BaudRate = baud;
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
            HAL_UART_DMAStop(ow_huart);
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
    if (HAL_UART_Receive_DMA(ow_huart, rx, len) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (HAL_UART_Transmit_DMA(ow_huart, (uint8_t *)tx, len) != HAL_OK)
    {
        HAL_UART_DMAStop(ow_huart);
        return HAL_ERROR;
    }
    return OW_WaitRx(OW_DMA_TIMEOUT);
}

/**
  * @brief  1-Wire reset + presence detect.
  * @retval 1 = at least one device answered, 0 = no presence pulse
  */
static uint8_t OW_Reset(void)
{
    uint8_t tx = 0xF0, rx = 0;

    OW_SetBaud(OW_BAUD_RESET);

    dbgResetStatus = OW_Transceive(&tx, &rx, 1);   /* DIAGNOSTIC */
	dbgResetRx = rx;                                /* DIAGNOSTIC */

	if (dbgResetStatus != HAL_OK)
	{
		OW_SetBaud(OW_BAUD_DATA);
		return 0;
	}
	OW_SetBaud(OW_BAUD_DATA);
	return (rx != 0xF0) ? 1 : 0;
}

/**
  * @brief  Single 1-Wire bit exchange. Send txBit (1 = release/read slot),
  *         return the bit sampled back.
  */
static uint8_t OW_Bit(uint8_t txBit)
{
    uint8_t tx = txBit ? 0xFF : 0x00;
    uint8_t rx = 0;
    HAL_StatusTypeDef st;   /* DIAGNOSTIC ONLY */

    st = OW_Transceive(&tx, &rx, 1);
    dbgLastByteStatus = st;                /* DIAGNOSTIC ONLY: every call */
    if (dbgByteIndex < 9)                  /* DIAGNOSTIC ONLY */
    {
        dbgByteStatus[dbgByteIndex++] = st;
    }

    if (st != HAL_OK)
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
    HAL_StatusTypeDef st;   /* DIAGNOSTIC ONLY */

    for (uint8_t i = 0; i < 8; i++)
    {
        txBuf[i] = (txByte & (1U << i)) ? 0xFF : 0x00;
    }

    st = OW_Transceive(txBuf, rxBuf, 8);   /* DIAGNOSTIC ONLY: capture status */
    dbgLastByteStatus = st;                /* DIAGNOSTIC ONLY */
    if (dbgByteIndex < 9)                  /* DIAGNOSTIC ONLY */
    {
        dbgByteStatus[dbgByteIndex++] = st;
    }
    if (st != HAL_OK)
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

    if (lastDeviceFlag)
    {
        return 0;
    }

    if (!OW_Reset())
    {
        lastDiscrepancy = 0;
        lastDeviceFlag = 0;
        lastFamilyDiscrepancy = 0;
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
            if (idBitNumber < lastDiscrepancy)
            {
                searchDirection = (romNo[romByteNumber] & romByteMask) ? 1 : 0;
            }
            else
            {
                searchDirection = (idBitNumber == lastDiscrepancy) ? 1 : 0;
            }
            if (searchDirection == 0)
            {
                lastZero = idBitNumber;
                if (lastZero < 9)
                {
                    lastFamilyDiscrepancy = lastZero;
                }
            }
        }

        if (searchDirection)
        {
            romNo[romByteNumber] |= romByteMask;
        }
        else
        {
            romNo[romByteNumber] &= (uint8_t)~romByteMask;
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
        lastDiscrepancy = lastZero;
        if (lastDiscrepancy == 0)
        {
            lastDeviceFlag = 1;
        }
        searchResult = 1;
    }

    if (!searchResult || (OW_CRC8(romNo, 7) != romNo[7]))
    {
        lastDiscrepancy = 0;
        lastDeviceFlag = 0;
        lastFamilyDiscrepancy = 0;
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

    LED_On(LED1); delayUs(30); LED_Off(LED1);
    if (!OW_Reset())
    {
        sensors[index].valid = 0;
        return;
    }

//    OW_ByteIO(DS18B20_CMD_MATCH_ROM);
    LED_On(LED1); delayUs(30); LED_Off(LED1);
    OW_ByteIO(DS18B20_CMD_SKIP_ROM);
//    for (uint8_t i = 0; i < 8; i++)
//    {
//        OW_ByteIO(sensors[index].rom[i]);
//    }
    LED_On(LED1); delayUs(30); LED_Off(LED1);
    OW_ByteIO(DS18B20_CMD_READ_SCRATCH);

    for (uint8_t i = 0; i < 9; i++)
    {
    	LED_On(LED1); delayUs(30); LED_Off(LED1);
        scratch[i] = OW_ByteIO(0xFF);
    }

    if (OW_CRC8(scratch, 8) != scratch[8])
    {
        sensors[index].valid = 0;
        return;
    }

    int16_t raw = (int16_t)((scratch[1] << 8) | scratch[0]);
    sensors[index].tempRaw = raw;
    sensors[index].valid = 1;
}

/* USER CODE END 0 */

/* Exported functions ----------------------------------------------------------*/
/* USER CODE BEGIN EF */

/**
  * @brief  Store the USART handle to use. Call once after MX_USART2_UART_Init().
  * @note   huart must already be configured for Single Wire (half-duplex) mode.
  */
void DS18B20_Init(UART_HandleTypeDef *huart)
{
    ow_huart = huart;
    sensorCount = 0;
    state = DS18B20_STATE_IDLE;
    dataReady = 0;
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
    lastDiscrepancy = 0;
    lastDeviceFlag = 0;
    lastFamilyDiscrepancy = 0;
    memset(romNo, 0, sizeof(romNo));

    while (OW_SearchROM() && (sensorCount < DS18B20_MAX_SENSORS))
    {
        memcpy(sensors[sensorCount].rom, romNo, 8);
        sensors[sensorCount].valid = 0;
        sensors[sensorCount].tempRaw = 0;
        sensorCount++;
    }

    return sensorCount;
}

/**
  * @brief  Broadcast Convert T to every sensor (Skip ROM) and switch the
  *         driver into non-blocking "waiting for conversion" state.
  *         Call DS18B20_Process() periodically afterwards.
  */
void DS18B20_StartConversion(void)
{
    if (state != DS18B20_STATE_IDLE)
    {
        return;
    }
    if (!OW_Reset())
    {
        return;
    }

    OW_ByteIO(DS18B20_CMD_SKIP_ROM);
    OW_ByteIO(DS18B20_CMD_CONVERT_T);

    convStartTick = HAL_GetTick();
    state = DS18B20_STATE_CONVERTING;
}

void DS18B20_Processing(int16_t *outRaw)
{
	uint8_t scratch[9];

	switch(DS18B20.MainStateMachine)
	{
		case 0:
			MX_USART1_UART_Init();
			DS18B20_Init(&huart1);
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
				LED_On(LED1); delayUs(3); LED_Off(LED1);
				dbgSkipRomEcho = OW_ByteIO(DS18B20_CMD_SKIP_ROM);
				LED_On(LED1); delayUs(3); LED_Off(LED1);
				dbgSkipRomStatus = dbgLastByteStatus;          /* DIAGNOSTIC ONLY */
				dbgReadScratchEcho = OW_ByteIO(DS18B20_CMD_READ_SCRATCH);
				dbgReadScratchStatus = dbgLastByteStatus;      /* DIAGNOSTIC ONLY */
				dbgByteIndex = 0;   /* DIAGNOSTIC ONLY: reset before the 9-byte read */
				for (uint8_t i = 0; i < 9; i++)
				{
					scratch[i] = OW_ByteIO(0xFF);
					LED_On(LED1); delayUs(3); LED_Off(LED1);
				}
				if (OW_CRC8(scratch, 8) != scratch[8])
				{
					DS18B20.MainStateMachine = 1;
					return; /* CRC mismatch */
				}
				*outRaw = (int16_t)((scratch[1] << 8) | scratch[0]);
				DS18B20.MainStateMachine++;
			}
			break;

		case 5:
			DS18B20.MainStateMachine++;
			break;

		case 6:
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
void DS18B20_Process(void)
{
    if (state != DS18B20_STATE_CONVERTING)
    {
        return;
    }

    if ((HAL_GetTick() - convStartTick) < DS18B20_CONV_TIME_MS)
    {
        return; /* not ready yet, come back later */
    }

    state = DS18B20_STATE_READING;

//    for (uint8_t i = 0; i < sensorCount; i++)
    for (uint8_t i = 0; i < 1; i++)
    {
        DS18B20_ReadOne(i);
    }

    dataReady = 1;
    state = DS18B20_STATE_IDLE;
}

uint8_t DS18B20_IsDataReady(void)
{
    return dataReady;
}

/**
  * @brief  Consumer calls this once it has actually read out the new
  *         values (e.g. via DS18B20_GetTemperatureRaw()). Until this is
  *         called, DS18B20_IsDataReady() keeps returning 1 even if a new
  *         conversion cycle has already started in the background —
  *         a completed reading is never silently missed just because the
  *         main loop wasn't fast enough to catch it on the exact iteration
  *         it became ready.
  */
void DS18B20_ClearDataReady(void)
{
    dataReady = 0;
}

DS18B20_State DS18B20_GetState(void)
{
    return state;
}

uint8_t DS18B20_GetSensorCount(void)
{
    return sensorCount;
}

int16_t DS18B20_GetTemperatureRaw(uint8_t index)
{
    if (index >= sensorCount || !sensors[index].valid)
    {
        return DS18B20_INVALID_RAW;
    }
    return sensors[index].tempRaw;
}

/**
  * @brief  Integer-only conversion: Q4 raw value (1 LSB = 1/16 degC) to
  *         hundredths of a degree Celsius (e.g. 2344 means 23.44 C).
  *         No float involved anywhere in this path.
  */
int32_t DS18B20_RawToCentiC(int16_t raw)
{
    return ((int32_t)raw * 100) / 16;
}

int32_t DS18B20_GetTemperatureCentiC(uint8_t index)
{
    int16_t raw = DS18B20_GetTemperatureRaw(index);
    if (raw == DS18B20_INVALID_RAW)
    {
        return INT32_MIN;
    }
    return DS18B20_RawToCentiC(raw);
}

const uint8_t *DS18B20_GetROM(uint8_t index)
{
    if (index >= sensorCount)
    {
        return NULL;
    }
    return sensors[index].rom;
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
    LED_On(LED1); delayUs(30); LED_Off(LED1);
    if (!OW_Reset())
    {
        return 0; /* no presence at all */
    }
//    LED_On(LED1); delayUs(30); LED_Off(LED1);

    OW_ByteIO(DS18B20_CMD_SKIP_ROM);
//    LED_On(LED1); delayUs(30); LED_Off(LED1);

    OW_ByteIO(DS18B20_CMD_CONVERT_T);
//    LED_On(LED1); delayUs(30); LED_Off(LED1);

    HAL_Delay(750); /* diagnostic only — blocking is fine here */

    if (!OW_Reset())
    {
        return 0; /* lost presence between convert and read */
    }
    OW_ByteIO(DS18B20_CMD_SKIP_ROM);
    OW_ByteIO(DS18B20_CMD_READ_SCRATCH);

    for (uint8_t i = 0; i < 9; i++)
    {
        scratch[i] = OW_ByteIO(0xFF);
        LED_On(LED1); delayUs(30); LED_Off(LED1);
    }
//    LED_On(LED1); delayUs(30); LED_Off(LED1);
    if (OW_CRC8(scratch, 8) != scratch[8])
    {
        return 0; /* CRC mismatch */
    }



    *outRaw = (int16_t)((scratch[1] << 8) | scratch[0]);
    return 1;
}

/**
  * @brief  HAL DMA RX complete callback. Weak override.
  * @note   If other modules also use USART DMA elsewhere in the project,
  *         switch to HAL_UART_RegisterCallback() instead of this weak
  *         override to avoid a symbol clash.
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        owRxDone = 1;
    }
}

/**
  * @brief  HAL DMA TX complete callback. Weak override.
  * @note   Same clash caveat as HAL_UART_RxCpltCallback above.
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        owTxDone = 1;
    }
}

/* USER CODE END EF */
