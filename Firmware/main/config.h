#ifndef CONFIG_H
#define CONFIG_H

/*
flash mode qio
flash size
关闭bootloader的flash检测 打开选项 Detect flash size when flashing bootloader
分区表 Partition Table partitions-ota_C3.csv
主任务栈 Main task stack size 改成 5120
watchdog 都关掉
关闭 Interrupt watchdog
优化等级 Compiler options / -o2
Log output 选择 waring
SPIFFS 名字长改成48字节
tick configTICK_RATE_HZ 改成1000
更改 LVGL printf stdio.h
LVGL font 字体 10 12 14 16 18 28 48
选择屏驱动 LCD controller model
LVGL Memory 改成38K
LVGL 屏幕刷新时间display refresh 5ms 按键读取时间 Input device read 100ms
coredump 设置到flash (暂时不用)
Make experimental features visible 勾选可加速flash，但对环境温度变化有20度要求（具体看官方文档）

Watch CPU0 Idle Task 去掉勾选
Channel for console 设置成 None // USB Serial/JTAG // USB CDC
Channel for console 设置成 USB_SERIAl_JTAT PORT // No secondary console
Local netif hostname 改成 LedController
CONFIG_LWIP_SNTP_MAX_SERVERS 改成 3
WebSocket server support   勾选
mqtt 勾引选 Enable MQTT protocol 5.0
Max HTTP Request Header Length 修改为 2048
*/

#ifndef CTRL_SW_VERSION
#define CTRL_SW_VERSION "1.3.2"
#endif

#ifdef TEST_VER // 测试标识
#define LED_CONTROLLER_VERSION CTRL_SW_VERSION "_Test"
#endif

#ifdef FACTORY_TEST_VER // 测试标识
#define LED_CONTROLLER_VERSION CTRL_SW_VERSION "_Factory"
#endif

#define DEVICE_NAME "LedController"

#ifndef LED_CONTROLLER_VERSION
#define LED_CONTROLLER_VERSION CTRL_SW_VERSION
#endif

#include "config_c3.h"

#define SNAIL_MQTT_URI "mqtt://climbsnail.cn"
#define SNAIL_MQTT_PORT 1883
#define SNAIL_MQTT_USERNAME "ClimbSnail"
#define SNAIL_MQTT_PASSWD "ClimbSnail.v0"

#define SNAIL_MQTT_DEVIVCE_ID_SIZE 24
#define SNAIL_MQTT_TOPIC_HEAD_SIZE 24
#define SNAIL_MQTT_LAST_WILL_TOP_SIZE (SNAIL_MQTT_TOPIC_HEAD_SIZE + 4)
#define SNAIL_MQTT_NORMAL_TOPIC_MAX_SIZE 48
#define SNAIL_MQTT_NORMAL_PAYLOAD_MAX_SIZE 16
#define SNAIL_MQTT_TOPIC_SYS_MANAGER "/led/manager" // 集群统一的主题
#define SNAIL_MQTT_TOPIC_SNAIL "/snail/%s/#"        // 主机的主题 138500865317602
#define SNAIL_MQTT_TOPIC_SYS "/sys"
#define SNAIL_MQTT_TOPIC_SOLDER "/solder"

#define SNAIL_MQTT_PAYLOAD_STARTUP "startup"
#define SNAIL_MQTT_PAYLOAD_OFFLINE "offline"
#define SNAIL_MQTT_PAYLOAD_WAKE "wake"
#define SNAIL_MQTT_PAYLOAD_SLEEP "sleep"

#endif