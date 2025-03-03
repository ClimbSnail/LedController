#ifndef _NETWORKER_DATA_H
#define _NETWORKER_DATA_H

#include <string>
using namespace std;

#define FORECAST_DAYS 4 // 天气预报的总天数

enum RUN_STATE_BIT : unsigned char
{
    RUN_STATE_BIT_WIFI = 0,   // wifi是否成功连接
    RUN_STATE_BIT_WEATHER,    // 天气是否成功获取
    RUN_STATE_BIT_TIME,       // 时间是否成功获取
    RUN_STATE_BIT_WEB_SERVER, // web配置服务是否已运行
    RUN_STATE_BIT_MQTT,       // MQTT服务是否已运行
    RUN_STATE_BIT_MAX
};
#define RUN_STATE_WIFI_BIT_MASK (1 << RUN_STATE_BIT_WIFI)
#define RUN_STATE_WEATHER_BIT_MASK (1 << RUN_STATE_BIT_WEATHER)
#define RUN_STATE_TIME_BIT_MASK (1 << RUN_STATE_BIT_TIME)
#define RUN_STATE_WEB_SERVER_BIT_MASK (1 << RUN_STATE_BIT_WEB_SERVER)
#define RUN_STATE_MQTT_BIT_MASK (1 << RUN_STATE_BIT_MQTT)

struct WeatherData
{
    int weatherCode;    // 天气现象代码
    int8_t temperature; // 温度
    int8_t humidity;    // 湿度
    int8_t maxTemp;     // 最高气温
    int8_t minTemp;     // 最低气温

    int airQulity;

    char cityname[20];  // 城市名
    char windDir[20];   // 风向
    char windpower[10]; // 风力
    char weather[25];   // 天气现象

    short daily_max[FORECAST_DAYS];
    short daily_min[FORECAST_DAYS];
};

struct WeatherALL
{
    uint8_t week;
    char dayweather[16];
    char nightweather[16];
    int8_t daytemp;
    int8_t nighttemp;
    char daywind[16];
    char nightwind[16];
    char daypower[16]; // 风力
};

struct TimeData
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

// 和api的枚举类型“wifi_mode_t”差不多
enum SH_WIFI_MODE : unsigned char
{
    SH_WIFI_MODE_CLOSE = 0, // 关闭
    SH_WIFI_MODE_STA,       // 开启STA模式
    SH_WIFI_MODE_AP,        // 开启AP模式
    SH_WIFI_MODE_STA_AP,    // 开启STA和AP模式
    SH_WIFI_MODE_MAX
};

// 联网管理器配置文件中的参数
struct NetworkerConfig
{
    SH_WIFI_MODE wifiMode;               // 网络连接模式
    unsigned char weatherEnable : 1;     // 天气是否开启（值类型 ENABLE_STATE）
    unsigned char webServerEnable : 1;   // 是否开启web配置服务器（值类型 ENABLE_STATE）
    unsigned char mqttEnable : 1;        // 是否开启mqtt服务（值类型 ENABLE_STATE）
    unsigned char _ : 5;                 // 预留
    char ssid_0[16];                     // 网络0名称
    string ssid_1;                       // 网络1名称
    string ssid_2;                       // 网络2名称
    char password_0[12];                 // 网络0密码
    char password_1[24];                 // 网络1密码
    char password_2[24];                 // 网络2密码
    char cityCode[8];                    // 城市名或代码
    char apiKey[36];                     // api的key
    char ignoredVer[12];                 // 已忽略的版本号
    char snailID[20];                    // 联动的主机id
    unsigned long weatherUpdataInterval; // 天气更新的时间间隔(s)
    unsigned long timeUpdataInterval;    // 日期时钟更新的时间间隔(s)
    unsigned int noticeId;               // 上次已读的公告id
};

enum USER_OTA_STATE : unsigned char
{
    USER_OTA_STATE_IDLE = 0,     // 空闲状态
    USER_OTA_STATE_REQUEST,      // 版本查询请求
    USER_OTA_STATE_REQUEST_WAIT, // 查询中
    USER_OTA_STATE_REQUEST_ERR,  // 查询异常
    USER_OTA_STATE_RESPONSE,     // 查询完毕
    USER_OTA_STATE_START,        // 开始升级
    USER_OTA_STATE_RUNNING,      // 升级中
    USER_OTA_STATE_RUNNING_ERR,  // 升级异常
    USER_OTA_STATE_END,          // 升级完成
    USER_OTA_STATE_MAX
};

enum UPDATA_LEVEL : unsigned char
{
    UPDATA_LEVEL_NONE = 0,      // 无升级
    UPDATA_LEVEL_UNABLE_DIRECT, // 无法直升
    UPDATA_LEVEL_ABLE,          // 可以升级
    UPDATA_LEVEL_RECOMMEND,     // 推荐升级
    UPDATA_LEVEL_FORCE,         // 强制升级
    UPDATA_LEVEL_MAX
};

enum UPDATE_PROGRESS : unsigned char
{
    UPDATE_PROGRESS_NONE = 0,  // 无状态
    UPDATE_PROGRESS_UPGRADING, // OTA升级中
    UPDATE_PROGRESS_SUCC,      // OTA成功
    UPDATE_PROGRESS_FAIL,      // OTA失败
    UPDATE_PROGRESS_MAX
};

#define OTA_RET_INFO_LEN 126
struct OtaData
{
    UPDATA_LEVEL updataLevel;       // 升级等级
    UPDATE_PROGRESS updateResult;   // 升级结果
    char retInfo[OTA_RET_INFO_LEN]; // 运行结果的明文表述
    string userId;                  // 用户id
    char nowVer[12];                // 当前版本号
    char newVer[12];                // 新版本号
    char firmwareUrl[96];           // 升级文件地址
    char firmwareMD5[128];          // 升级文件md5值
};

#endif // _NETWORKER_DATA_H