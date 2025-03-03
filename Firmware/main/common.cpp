#include "common.h"
#include "sh_driver/knobs.h"

#include "esp_partition.h"
#include "esp_adc_cal.h"
// #include "esp_adc/adc_cali.h"    // IDF5.0
// #include "esp_adc/adc_cali_scheme.h"
// #include "libb64/cdecode.h"
#include "mbedtls/base64.h"
#include "sh_hal/hal.h"
#include "driver/gpio.h"
#include "driver/timer_types_legacy.h"

FlashFS g_flashCfg;                               // flash中的文件系统（替代原先的Preferences）
InteriorNTC interior_ntc;                         // 内部ntc
Buzzer buzzer(BEEP_PIN, 50);                      // 蜂鸣器
Knobs knobs(EC11_A_PIN, EC11_B_PIN, EC11_SW_PIN); // EC11编码器

// 数据持久化操作的锁
SemaphoreHandle_t sh_update_cfg_mutex = xSemaphoreCreateMutex();
// 拓展板串口通信锁
SemaphoreHandle_t sh_ext_uart_mutex = xSemaphoreCreateMutex();

bool doDelayMillisTime(uint32_t interval, uint32_t *previousMillis, bool state)
{
    uint32_t currentMillis = GET_SYS_MILLIS();
    if (currentMillis - *previousMillis >= interval)
    {
        *previousMillis = currentMillis;
        state = !state;
    }
    return state;
}

string base64_encode(const uint8_t *data, size_t length)
{
    unsigned char buf[256];
    size_t olen = 0;
    mbedtls_base64_encode(buf, 256, &olen,
                          data, length);
    buf[olen] = '\0'; // 确保字符串以null结尾

    return string((char *)buf);
}

/**
 * convert input data to base64
 * @param data const uint8_t *
 * @param length size_t
 * @return String
 */
int base64_decode(const uint8_t *data, size_t length, unsigned char *decode_buffer, uint16_t buf_len)
{
    size_t olen = 0;

    // 计算编码后的长度
    mbedtls_base64_decode(decode_buffer, buf_len, &olen, data, length);

    // 在buf中存储了Base64编码后的字符串，长度为olen
    decode_buffer[olen] = '\0'; // 确保字符串以null结尾

    return olen;
}

/**
 * convert input data to base64
 * @param text const String&
 * @return String
 */
int base64_decode(const string &text, unsigned char *decode_buffer, uint16_t buf_len)
{
    return base64_decode((uint8_t *)text.c_str(), text.length(), decode_buffer, buf_len);
}

#define MAX_CORE_ID 16 // 发热芯最大编号

int16_t find_first_nonzero_ind(uint16_t bitFlag, uint16_t startInd)
{
    uint16_t index = startInd;

    if (index >= MAX_CORE_ID)
    {
        return -1;
    }

    bitFlag >>= index;

    while (!(bitFlag & 0x01))
    {
        ++index;
        if (index >= MAX_CORE_ID)
        {
            return -1;
        }
        bitFlag >>= 1;
    }

    return index;
}

int16_t find_first_zero_ind(uint16_t bitFlag, uint16_t startInd)
{
    uint16_t index = startInd;

    if (index >= MAX_CORE_ID)
    {
        return -1;
    }

    bitFlag >>= index;

    while (bitFlag & 0x01)
    {
        ++index;
        if (index >= MAX_CORE_ID)
        {
            return -1;
        }
        bitFlag >>= 1;
    }

    return index;
}