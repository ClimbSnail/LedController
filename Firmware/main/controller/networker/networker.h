#ifndef __NETWORKER_H
#define __NETWORKER_H

#include "controller_base.h"
#include "networker_data.h"
#include "network.h"
#include "controller/LedController/LedController.h"

extern unsigned char g_webSaveConfigFlag; // 保存配置标志位

/**********************************
 * 联网管理器类
 */
class Networker : public ControllerBase
{

public:
    NetworkerConfig m_utilConfig;      // 网络的通用信息
    NetworkerConfig m_utilConfigCache; // 网络的通用信息(运行使用)

    unsigned char m_runState = 0; // 各个功能的运行状态
    int8_t m_wifiRssi;            // WiFi信号强度
    char m_staIpStr[16];          // IP地址字符串
    TimeData m_timeData;          // 时间数据
    OtaData m_otaData;            // 固件升级数据
    USER_OTA_STATE m_otaState;    // 固件升级状态
    LedUtilConfig *ledUtilConfig;

public:
    Networker(const char *name, SnailManager *m_manager);
    ~Networker();
    bool Start();
    bool Process();
    bool End();

    bool ManageConfig(INFO_MANAGE_ACTION *action);
    // 消息处理
    bool MessageHandle(const char *from, const char *to,
                       CTRL_MESSAGE_TYPE type, void *message,
                       void *ext_info);
    bool OtaProcess();
    void OtaHandle(USER_OTA_STATE *state);
    bool GetWeatherInfo(char *info);
    void WriteConfig(NetworkerConfig *cfg);
    void ReadConfig(NetworkerConfig *cfg);
    bool resetUtilConfig(void);
    void WriteConfig(WeatherData *cfg);
    void ReadConfig(WeatherData *cfg);
    bool UpdateConfig(void);
    bool SetLedCfgPoint(LedUtilConfig *ledUtilCfg);
};

#endif