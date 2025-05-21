#ifndef COMMON_H
#define COMMON_H

#include <functional>
#include "config.h"
#include "esp_log.h"
#include <string>

using namespace std;

/* Number to string macro */
#define _VERSION_NUM_TO_STR_(n) #n
#define VERSION_NUM_TO_STR(n) _VERSION_NUM_TO_STR_(n)

// #define SH_LOG(format, ...) UART_PORT.printf("[SnailManager]" format "\r\n", ##__VA_ARGS__)
#define SH_LOGE(format, ...) ESP_LOGE("[SnailManager]", format, ##__VA_ARGS__)
#define SH_LOG(format, ...) ESP_LOGW("", format, ##__VA_ARGS__)
#define SH_LOG_DEBUG(format, ...) ESP_LOGW("", format, ##__VA_ARGS__)
// #define SH_LOG(format, ...)
// #define SH_LOG_DEBUG(format, ...)

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
// #include <Preferences.h>

#include "controller_base.h"

#include "sh_driver/flash_fs.h"
#include "sh_driver/interior_ntc.h"
#include "sh_driver/knobs.h"
#include "sh_driver/buzzer.h"
// #include "freertos/semphr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 数据持久化 操作的锁
extern SemaphoreHandle_t sh_update_cfg_mutex;
// 数据持久化操作的安全宏（写数据错误）
#define SH_UPDATE_CFG_OPERATE_LOCK(CODE)                              \
    if (pdTRUE == xSemaphoreTake(sh_update_cfg_mutex, portMAX_DELAY)) \
    {                                                                 \
        CODE;                                                         \
        xSemaphoreGive(sh_update_cfg_mutex);                          \
    }

extern FlashFS g_flashCfg;       // flash中的文件系统（替代原先的Preferences）
extern InteriorNTC interior_ntc; // 内部ntc
extern Knobs knobs;              // EC11编码器
extern Buzzer buzzer;            // 蜂鸣器

bool doDelayMillisTime(uint32_t interval,
                       uint32_t *previousMillis,
                       bool state);
string base64_encode(const uint8_t *data, size_t length);
int base64_decode(const uint8_t *data, size_t length,
                  unsigned char *decode_buffer, uint16_t buf_len);
int base64_decode(const string &text, unsigned char *decode_buffer, uint16_t buf_len);

// 查找第一个非0的位所在的下标
int16_t find_first_nonzero_ind(uint16_t bitFlag, uint16_t startInd);
// 查找第一个 位值为0的位所在的下标
int16_t find_first_zero_ind(uint16_t bitFlag, uint16_t startInd);
// 设置位 bitIndex>=0
#define setBit(value, bitIndex) \
    value = value | (0x01 << bitIndex)
// 清除位 bitIndex>=0
#define clearBit(value, bitIndex) \
    value = ~(0x01 << coreID) & value

string getMachineCode(void); // 获取机器码

bool Shutdown();

#endif