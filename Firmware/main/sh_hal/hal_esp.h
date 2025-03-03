#ifndef __HAL_ESP_H
#define __HAL_ESP_H

#include <stdint.h>
#include <functional>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "hal/adc_types.h"

namespace SH_HAL
{
    enum SH_GPIO_MODE : unsigned char
    {
        SH_GPIO_MODE_INPUT = GPIO_MODE_INPUT,
        SH_GPIO_MODE_OUTPUT = GPIO_MODE_OUTPUT,
        SH_GPIO_MODE_INPUT_OUTPUT = GPIO_MODE_INPUT_OUTPUT,
        SH_GPIO_MODE_OPEN_DRAIN = GPIO_MODE_INPUT_OUTPUT_OD,
        SH_GPIO_MODE_OUTPUT_OPEN_DRAIN = GPIO_MODE_OUTPUT_OD,

        SH_GPIO_MODE_MAX
    };

    enum SH_GPIO_PULL : unsigned char
    {
        SH_GPIO_PULL_FLOATING = 0x00,
        SH_GPIO_PULL_UP_ONLY = 0x01,
        SH_GPIO_PULL_DOWN_ONLY = 0x02,
        SH_GPIO_PULL_UP_DOWN = 0x03,

        SH_GPIO_PULL_MAX
    };

    enum SH_GPIO_LEVEL : unsigned char
    {
        SH_GPIO_LEVEL_LOW,
        SH_GPIO_LEVEL_HIGH,
        SH_GPIO_LEVEL_UNDEF
    };

    enum SH_GPIO_INTR_TYPE : unsigned char
    {
        SH_GPIO_INTR_TYPE_DISABLE = GPIO_INTR_DISABLE,       /*!< Disable GPIO interrupt                             */
        SH_GPIO_INTR_TYPE_POSEDGE = GPIO_INTR_POSEDGE,       /*!< GPIO interrupt type : rising edge                  */
        SH_GPIO_INTR_TYPE_NEGEDGE = GPIO_INTR_NEGEDGE,       /*!< GPIO interrupt type : falling edge                 */
        SH_GPIO_INTR_TYPE_ANYEDGE = GPIO_INTR_ANYEDGE,       /*!< GPIO interrupt type : both rising and falling edge */
        SH_GPIO_INTR_TYPE_LOW_LEVEL = GPIO_INTR_LOW_LEVEL,   /*!< GPIO interrupt type : input low level trigger      */
        SH_GPIO_INTR_TYPE_HIGH_LEVEL = GPIO_INTR_HIGH_LEVEL, /*!< GPIO interrupt type : input high level trigger     */
        SH_GPIO_INTR_TYPE_MAX = GPIO_INTR_MAX,
    };

    enum SH_ADC_ATTEN : unsigned char
    {
        SH_ADC_ATTEN_0db = ADC_ATTEN_DB_0,
        SH_ADC_ATTEN_2_5db = ADC_ATTEN_DB_2_5,
        SH_ADC_ATTEN_6db = ADC_ATTEN_DB_6,
        SH_ADC_ATTEN_11db = ADC_ATTEN_DB_11,
        SH_ADC_ATTEN_MAX
    };

    uint32_t getSysFreq();
    uint64_t getChipID();

    bool halInit(void);

    bool gpioInit(uint8_t pinNum, SH_GPIO_MODE mode, SH_GPIO_PULL pullMode,
                  SH_GPIO_LEVEL initLevel = SH_GPIO_LEVEL_UNDEF);
    bool gpioLevelSet(uint8_t pinNum, SH_GPIO_LEVEL level);
    uint8_t gpioLevelRead(uint8_t pinNum);

    bool gpioSetIntr(uint8_t pinNum, void (*gpio_isr_handler)(void *),
                     SH_GPIO_INTR_TYPE interruptType, void *param = NULL);
    bool gpioDisableIntr(uint8_t pinNum);

    bool pwmTest(void);
    uint32_t pwmInit(uint8_t pinNum, uint32_t freq, uint8_t bitNum);
    bool pwmEnable(uint8_t pinNum, bool enable, SH_GPIO_LEVEL idleLevel = SH_GPIO_LEVEL_UNDEF);
    bool pwmSetDuty(uint8_t pinNum, uint32_t duty);
    uint32_t pwmSetFreq(uint8_t pinNum, uint32_t freq);

    bool adcInit(uint8_t pinNum, SH_ADC_ATTEN attenuation);
    bool adcSetAtten(uint8_t pinNum, SH_ADC_ATTEN attenuation);
    uint32_t adcRead(uint8_t pinNum, bool do_calibration = false);

    void touchInit();
    uint32_t touchRead();

    bool uartInit(uint32_t baud);
    int uartAvailable();
    size_t uartRead(uint8_t *data, size_t size, uint32_t timeout = 10);
    size_t uartWrite(const uint8_t *data, size_t size);

    bool addZeroCheckCb(void (*handler)(void *), void *param);
    void runZeroCheckCb(void *param);

    bool generatePulse(void *param);

    bool dacInit(uint8_t pinNum);
    bool dacSetVol(uint8_t pinNum, uint16_t dacVoltage);

    void continuous_adc_init();
    void oneshot_adc_init();
}

#endif
