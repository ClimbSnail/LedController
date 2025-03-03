#ifndef CONFIG_SUB_H
#define CONFIG_SUB_H

#define GET_SYS_MILLIS xTaskGetTickCount // 获取系统毫秒数
// #define GET_SYS_MILLIS millis            // 获取系统毫秒数

#define SYS_POWER_VOL 3300.0 // 系统的供电电压
// #define ADC_11DB_VREF 2516 // ADC 11db的基准电压
#define ADC_25DB_VREF 1100 // ADC 2.5db的基准电压
#define ADC_11DB_VREF 2600 // ADC 11db的基准电压
// #define ADC_11DB_VREF 1350 // ADC 11db的基准电压

#define EMPTY_PIN 255 // 定义空引脚

#define POWER_EN 20 // 电源维持使能
#define WORK_LED 21 // 开机指示灯
#define KEY_SW 4    // 普通按键同时也是电源开启按键
#define POWER_ADC 0 // 电源电压
#define NTC_ADC 1   // NTC电阻采集
#define LED_PWM_0 5
#define LED_PWM_1 6
#define LED_PWM_2 7
#define LED_PWM_3 8
#define FAN_PWM 10
#define BOOT 9

#define KEY_TOUCH_0 2
#define KEY_TOUCH_1 3
#define KEY_TOUCH_2 4

// 编码器
#define EC11_A_PIN 2
#define EC11_B_PIN 3
#define EC11_SW_PIN KEY_SW

#define BEEP_PIN 4

// pwm通道
#define PWM_0_CHANNEL 0
#define PWM_1_CHANNEL 1
#define PWM_2_CHANNEL 2
#define PWM_3_CHANNEL 3
#define FAN_CHANNEL 4

// pwm通道组的频率 两个通道将会使用同一组配置
// #define PWM_GROUP_0_FREQ 50
// // #define PWM_GROUP_0_FREQ 4096
// #define PWM_GROUP_0_BIT_WIDTH 12
// #define PWM_GROUP_0_UNIT_VAL 4.095
// #define PWM_GROUP_0_UNIT_COUNT 1000
#define PWM_GROUP_0_FREQ 22000
#define PWM_GROUP_0_BIT_WIDTH 10
#define PWM_GROUP_0_UNIT_VAL 1.023
#define PWM_GROUP_0_UNIT_COUNT 1000

// #define PWM_GROUP_1_FREQ 50
#define PWM_GROUP_1_FREQ 4096
#define PWM_GROUP_1_BIT_WIDTH 12
#define PWM_GROUP_1_UNIT_VAL 4.095
#define PWM_GROUP_1_UNIT_COUNT 1000
// #define PWM_GROUP_1_FREQ 22000
// #define PWM_GROUP_1_BIT_WIDTH 10
// #define PWM_GROUP_1_UNIT_VAL 1.023
// #define PWM_GROUP_1_UNIT_COUNT 1000

#define PWM_GROUP_2_FREQ 64
#define PWM_GROUP_2_BIT_WIDTH 11
#define PWM_GROUP_2_UNIT_VAL 20.47
#define PWM_GROUP_2_UNIT_COUNT 100

#define SH_SCREEN_WIDTH 280
#define SH_SCREEN_HEIGHT 240

// #define UART_PORT Serial0               // 串口号
#define UART_BAUD 115200             // 115200 / 460800 / 921600
#define ADC_WIDTH_BITS ((uint8_t)12) // ADC的采集分辨率
#define ADC_VOL_MAX_VALUE 2450       // ADC检测热电偶的最大电压值mv

// 核心id <=portNUM_PROCESSORS-1
#define TASK_WIFI_CORE_ID 0        // WIFI任务所在的核心
#define TASK_BEEP_CORE_ID 0        // 蜂鸣器任务所在的核心
#define TASK_MANAGER_BG_CORE_ID 0  // 管理后台进程的任务所在的核心
#define TASK_MANAGER_UI_CORE_ID 0  // 管理器更新UI进程的任务所在的核心
#define TASK_SGM_CORE_ID 0         // 独立ADC进程的任务所在的核心
#define TASK_RGB_CORE_ID 0         // RGB的任务所在的核心
#define TASK_LVGL_CORE_ID 0        // LVGL的页面所在的核心
#define TASK_SPOT_WELDER_CORE_ID 0 // 点焊机任务所在的核心
#define TASK_HTTPD_CORE_ID 0       // HTTPD任务所在的核心
#define TASK_WRB_SERVER_CORE_ID 0  // WebServer任务所在的核心

// 优先级定义(数值越小优先级越低)
// 最高为 configMAX_PRIORITIES-1
#define TASK_WIFI_PRIORITY 1        // WIFI任务优先级
#define TASK_BEEP_PRIORITY 2        // 蜂鸣器任务优先级
#define TASK_MANAGER_BG_PRIORITY 5  // 管理后台进程的任务优先级
#define TASK_MANAGER_UI_PRIORITY 3  // 管理器更新UI进程的任务优先级
#define TASK_SGM_PRIORITY 3         // 独立ADC进程的任务优先级
#define TASK_TOUCH_PRIORITY 6       // 触摸按键进程的任务优先级
#define TASK_RGB_PRIORITY 2         // RGB的任务优先级
#define TASK_LVGL_PRIORITY 2        // LVGL的页面优先级
#define TASK_SPOT_WELDER_PRIORITY 3 // 点焊机任务优先级
#define TASK_WRB_SERVER_RIORITY 1   // WebServer任务优先级

// #define USER_PWM_CTRL_SRCEEN // 定时器不够 不使用

#define TFT_RES_PIN 9
#define TFT_DC_PIN 13
#define TFT_SCK_PIN 12
#define TFT_MOSI_PIN 11
#define TFT_MISO_PIN -1
#define TFT_CS_PIN 10

#define NETWORKABLE

#endif