/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ntc_termistor.c
  * @brief          : module_name driver / logic implementation
  * Tested with NTCM-HP-33K-1%
  * https://voron.ua/uk/catalog/022176--termistor_ntc_33k_1_50mw_ntcm-hp-33k-1
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes --------------------------------------------------------------------*/
#include "ntc_termistor.h"
#include <stdint.h>
#include <stdlib.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Константы термистора NTCM-HP-33K-1%
#define B_COEFFICIENT   3950.0f  // Бета-коэффициент
#define R0_NOMINAL      33000.0f // Номинальное сопротивление при 25C (33 кОм)
#define T0_NOMINAL      298.15f  // Номинальная температура в Кельвинах (25C + 273.15)
#define R_PULLUP        33000.0f // Номинал постоянного резистора делителя (33 кОм)
#define ADC_MAX_VAL     4095.0f  // 12-битный ADC
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes ------------------------------------------------*/
/* USER CODE BEGIN PFP */
static int32_t int_ln_q16(int32_t x_q16);
/* USER CODE END PFP */

/* Private user code -----------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* Exported functions ----------------------------------------------------------*/
/* USER CODE BEGIN EF */
/**********************************************************************
  * @brief
  * @param  None
  * @retval None
   ********************************************************************/
void NTC_Init(void)
{

}


/**
 * @brief  Высокоточный целочисленный натуральный логарифм ln(x).
 *         Входное значение x должно быть масштабировано как (x * 65536) (формат Q16).
 *         Возвращает ln(x), масштабированный на 65536 (формат Q16).
 */
static int32_t int_ln_q16(int32_t x_q16)
{
    if (x_q16 <= 0) return 0;

    int32_t log_val = 0;

    // Приводим x к диапазону [0.5, 1.0] для высокой точности ряда
    while (x_q16 < 32768) {
        x_q16 <<= 1;
        log_val -= 45426; // ln(2) * 65536 = 45426
    }
    while (x_q16 >= 65536) {
        x_q16 >>= 1;
        log_val += 45426; // ln(2) * 65536 = 45426
    }

    // Аппроксимация ряда Тейлора для ln(x) в окрестности 1
    // y = (x - 1) / (x + 1)
    int32_t num = x_q16 - 65536;
    int32_t den = x_q16 + 65536;
    int32_t y_q16 = (num << 16) / den;

    int32_t y2_q16 = (y_q16 * y_q16) >> 16;

    // Считаем первые три члена ряда: 2 * (y + y^3/3 + y^5/5)
    int32_t sum = y_q16;
    int32_t y3 = (y_q16 * y2_q16) >> 16;
    sum += y3 / 3;
    int32_t y5 = (y3 * y2_q16) >> 16;
    sum += y5 / 5;

    log_val += (sum << 1);
    return log_val;
}


/**
 * @brief  Расчет температуры по уравнению Стейнхарта-Харта (Бета-форма)
 *         Полностью в целых числах без использования таблиц.
 * @param  adc_val: Сырое или усредненное значение ADC (0...4095)
 * @retval Температура, умноженная на 100 (например, 2750 = 27.50 °C)
 */
int32_t NTC_GetTemperature(int32_t adc_val)
{
    // Защита от критических значений (обрыв датчика или КЗ)
    if (adc_val <= 10)   return -4000;
    if (adc_val >= 4085) return 15000;

    /*
     * 1. Находим отношение R_ntc / R0.
     * При вашей схеме (NTC к 3.3V, R_pull к GND):
     * R_ntc = R_pull * (4095 - ADC) / ADC.
     * Так как R_pull = R0 = 33 кОм, они сокращаются!
     * Отношение R_ntc / R0 = (4095 - adc_val) / adc_val.
     */
    int32_t ratio_q16 = ((4095 - adc_val) << 16) / adc_val;

    /*
     * 2. Вычисляем натуральный логарифм ln(R_ntc / R0) в формате Q16
     */
    int32_t ln_ratio_q16 = int_ln_q16(ratio_q16);

    /*
     * 3. Уравнение Стейнхарта-Харта через Бету:
     * 1/T = 1/T0 + ln(R/R0) / Beta
     *
     * Переведем константы в фиксированную запятую с большим масштабом (Q30), чтобы избежать потери точности:
     * 1/T0 (для 25°C) = 1 / 298.15 = 0.003354016 -> умножаем на 2^30 = 3601343
     * 1/Beta = 1 / 3950 = 0.000253164 -> умножаем на 2^30 = 271833
     */
    int64_t inv_T0_q30 = 3601343LL;
    int64_t inv_Beta_q30 = 271833LL;

    // Считаем 1/T в формате Q30
    // Так как ln_ratio в Q16, сдвигаем результат умножения на 16 бит вправо
    int64_t inv_T_q30 = inv_T0_q30 + ((ln_ratio_q16 * inv_Beta_q30) >> 16);

    // Получаем Кельвины: T = 2^30 / inv_T_q30
    // Умножаем на 100 прямо при делении, чтобы сразу получить сотые доли градуса
    int32_t temp_kelvin_x100 = (int32_t)((1073741824LL * 100) / inv_T_q30);

    // Переводим в Цельсии (*100): T_c = T_k - 273.15
    int32_t temp_celsius_x100 = temp_kelvin_x100 - 27315;

    return temp_celsius_x100;
}

/**********************************************************************
  * @brief  Выключить светодиод.
  * @param  None
  * @retval None
   ********************************************************************/
void NTC_GetTemperature1(void)
{

}
/* USER CODE END EF */
