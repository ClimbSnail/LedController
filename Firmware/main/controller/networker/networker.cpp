#include "networker.h"
#include "network.h"
#include "netota.h"
#include "web_server.h"
#include "common.h"

#include "sh_hal/hal.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "sntp.h"
#include "sh_mqtt.h"

#ifdef __cplusplus
} /* extern "C" */
#endif
#include <time.h>
#include <sys/time.h>

unsigned char g_webSaveConfigFlag = false; // WebServer触发的保存配置标志位

Networker::Networker(const char *name, SnailManager *m_manager)
    : ControllerBase(name, CTRL_TYPE_NETWORKER, m_manager)
{
    m_enableFlag = ENABLE_STATE_CLOSE;
    m_initFlag = false;

    strncpy(m_staIpStr, "0.0.0.0", sizeof(m_staIpStr));

    m_otaData.updataLevel = UPDATA_LEVEL_NONE;
    m_otaData.updateResult = UPDATE_PROGRESS_NONE;
    memset(m_otaData.retInfo, 0, sizeof(m_otaData.retInfo));
    snprintf(m_otaData.nowVer, sizeof(m_otaData.nowVer), "%s", LED_CONTROLLER_VERSION);
    // m_otaData.userId = SH_HAL::getChipID();
    m_otaData.userId = getMachineCode(); // m_manager->getMachineCode();

    m_runState = 0x00;
    m_wifiRssi = WIFI_RSSI_MIN_DISCONNECT;
    m_otaState = USER_OTA_STATE_IDLE;
    // 初始时间
    m_timeData.tm_year = constrain(2024, 1997, 3000);
    m_timeData.tm_mon = constrain(10, 0, 11);
    m_timeData.tm_mday = constrain(2, 1, 31);
    m_timeData.tm_wday = constrain(0, 0, 6);
    m_timeData.tm_hour = constrain(12, 0, 23);
    m_timeData.tm_min = constrain(0, 0, 59);
    m_timeData.tm_sec = constrain(0, 0, 59);
}

Networker::~Networker()
{
    this->End();
}

bool Networker::Start()
{
    this->ReadConfig(&m_utilConfig);
    m_utilConfigCache = m_utilConfig; // 备份到缓存

    m_initFlag = true;

    return true;
}

bool Networker::SetCtrlCfgPoint(LedUtilConfig *ledUtilCfg)
{
    ledUtilConfig = ledUtilCfg;
    return true;
}

bool Networker::Process()
{
    // WebServer处理
    if (SH_WIFI_MODE_CLOSE != m_utilConfigCache.wifiMode &&
        ENABLE_STATE_OPEN == m_utilConfigCache.webServerEnable)
    {
        m_runState &= (~RUN_STATE_WIFI_BIT_MASK); // 提前清零标志位
        if (!(m_runState & RUN_STATE_WEB_SERVER_BIT_MASK))
        {
            sh_wifi_init_softap();
            // WIFI_MODE_APSTA WIFI_MODE_STA WIFI_MODE_AP
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

            m_runState |= RUN_STATE_WEB_SERVER_BIT_MASK; // 提前使能标志位
            web_server_init(&m_utilConfigCache, ledUtilConfig);
        }
    }
    else if (SH_WIFI_MODE_CLOSE != m_utilConfigCache.wifiMode)
    {
        m_runState &= (~RUN_STATE_WEB_SERVER_BIT_MASK); // 提前清零标志位
        if (!(m_runState & RUN_STATE_WIFI_BIT_MASK) ||
            WIFI_RSSI_MIN_DISCONNECT == m_wifiRssi)
        {
            // m_utilConfigCache.ssid_1 = "HQ";
            // snprintf(m_utilConfigCache.password_1, sizeof(m_utilConfigCache.password_1), "%s", "A773181861a");
            // snprintf(m_utilConfigCache.apiKey, sizeof(m_utilConfigCache.apiKey), "%s", "b4f0b852ba750edb4432a93e8f412141");
            // snprintf(m_utilConfigCache.cityCode, sizeof(m_utilConfigCache.cityCode), "%s", "110000"); // 北京 110000

            bool ret = false;
            ret = sh_wifi_init_sta((wifi_mode_t)m_utilConfigCache.wifiMode,
                                   m_utilConfigCache.ssid_1.c_str(), m_utilConfigCache.password_1,
                                   m_utilConfigCache.ssid_2.c_str(), m_utilConfigCache.password_2,
                                   m_utilConfigCache.ssid_0, m_utilConfigCache.password_0);

            char sysInfo[128];
            if (true == ret)
            {
                m_runState |= RUN_STATE_WIFI_BIT_MASK;
                strncpy(m_staIpStr, get_ip_str(), sizeof(m_staIpStr));
                // m_wifiRssi = sh_get_connect_rssi();
                snprintf(sysInfo, 128, "WIFI连接成功. STA IP: %s", m_staIpStr);
                SH_LOG("%s", sysInfo);

                // // long long timestamp = get_timestamp(TIME_API); // nowapi时间API
                // get_weather(m_utilConfigCache.apiKey,
                //             m_utilConfigCache.cityCode);

                // 天气获取成功标志位
                m_runState |= RUN_STATE_WEATHER_BIT_MASK;

                tm timeInfo = getTimeInfo();
                m_timeData.tm_year = constrain(timeInfo.tm_year, 1997, 3000);
                m_timeData.tm_mon = constrain(timeInfo.tm_mon, 0, 11);
                m_timeData.tm_mday = constrain(timeInfo.tm_mday, 1, 31);
                m_timeData.tm_wday = constrain(timeInfo.tm_wday, 0, 6);
                m_timeData.tm_hour = constrain(timeInfo.tm_hour, 0, 23);
                m_timeData.tm_min = constrain(timeInfo.tm_min, 0, 59);
                m_timeData.tm_sec = constrain(timeInfo.tm_sec, 0, 59);
                // 时间获取成功标志位
                m_runState |= RUN_STATE_TIME_BIT_MASK;
            }
        }
    }
    else
    {
        m_runState &= (~RUN_STATE_WIFI_BIT_MASK);       // 提前清零Wifi标志位
        m_runState &= (~RUN_STATE_WEB_SERVER_BIT_MASK); // 提前清零Web服务标志位
    }

    m_wifiRssi = sh_get_connect_rssi();
    // if (!(m_runState & RUN_STATE_WIFI_BIT_MASK))
    // {
    //     m_wifiRssi = WIFI_RSSI_MIN_DISCONNECT;
    // }

    m_utilConfigCache.mqttEnable = ENABLE_STATE_OPEN;
    // MQTT处理
    if (WIFI_RSSI_MIN_DISCONNECT != m_wifiRssi &&
        (!(m_runState & RUN_STATE_MQTT_BIT_MASK)) &&
        ENABLE_STATE_OPEN == m_utilConfigCache.mqttEnable)
    {
        m_runState |= RUN_STATE_MQTT_BIT_MASK; // 提前使能标志位
        // mqttStart("SnailID");
        mqttStart(getMachineCode().c_str(), m_utilConfig.snailID);
    }

    if (true == g_webSaveConfigFlag)
    {
        UpdateConfig();
        g_webSaveConfigFlag = false;
    }

    vTaskDelay(30);
    if ((m_runState & RUN_STATE_TIME_BIT_MASK))
    {
        time_t now = 0;
        tm timeInfo;
        time(&now);
        // Set timezone to China Standard Time
        setenv("TZ", "CST-8", 1);
        tzset();
        localtime_r(&now, &timeInfo); // 本地时区时间
        // gmtime_r(&now, &timeInfo);    // 世界时钟
        m_timeData.tm_year = constrain(timeInfo.tm_year, 1997, 3000);
        m_timeData.tm_mon = constrain(timeInfo.tm_mon, 0, 11);
        m_timeData.tm_mday = constrain(timeInfo.tm_mday, 1, 31);
        m_timeData.tm_wday = constrain(timeInfo.tm_wday, 0, 6);
        m_timeData.tm_hour = constrain(timeInfo.tm_hour, 0, 23);
        m_timeData.tm_min = constrain(timeInfo.tm_min, 0, 59);
        m_timeData.tm_sec = constrain(timeInfo.tm_sec, 0, 59);
    }
    return true;
}

bool Networker::End()
{
    return true;
}
bool Networker::OtaProcess()
{
    // OTA升级
    if ((m_runState & RUN_STATE_WIFI_BIT_MASK) &&
        USER_OTA_STATE_RUNNING == m_otaState)
    {
        switch (m_otaData.updateResult)
        {
        case UPDATE_PROGRESS_NONE:
        {
            SH_LOG("Start OTA");
            ota_start(&m_otaData);
        }
        break;
        case UPDATE_PROGRESS_UPGRADING:
        {
            // snprintf(m_otaData.retInfo, OTA_RET_INFO_LEN, MASSAGE_INFO_UPGRADING);
        }
        break;
        case UPDATE_PROGRESS_SUCC:
        {
            m_otaState = USER_OTA_STATE_END;
            m_otaData.updateResult = UPDATE_PROGRESS_NONE;
            // snprintf(m_otaData.retInfo, OTA_RET_INFO_LEN, MASSAGE_INFO_UPGRADING_SUCC_REBOOT);
            vTaskDelay(5000);
            esp_restart();
        }
        break;
        case UPDATE_PROGRESS_FAIL:
        {
            // 升级失败
            m_otaState = USER_OTA_STATE_RUNNING_ERR;
            m_otaData.updateResult = UPDATE_PROGRESS_NONE;
            // snprintf(m_otaData.retInfo, OTA_RET_INFO_LEN, MASSAGE_INFO_UPGRADING_FAIL);
        }
        break;
        default:
            break;
        }
    }

    // 固件查询
    if ((m_runState & RUN_STATE_WIFI_BIT_MASK) &&
        USER_OTA_STATE_REQUEST_WAIT == m_otaState)
    {
        if (true == get_updata_info(&m_otaData))
        {
            // SH_LOG("m_otaData.updataLevel = %u", m_otaData.updataLevel);
            // SH_LOG("m_otaData.newVer = %s", m_otaData.newVer);
            // SH_LOG("m_otaData.firmwareUrl = %s", m_otaData.firmwareUrl);
            // SH_LOG("m_otaData.firmwareMD5 = %s", m_otaData.firmwareMD5);
            m_otaState = USER_OTA_STATE_RESPONSE;
            switch (m_otaData.updataLevel)
            {
            case UPDATA_LEVEL_NONE:
            {
                // snprintf(m_otaData.retInfo, OTA_RET_INFO_LEN, MASSAGE_INFO_NOTHING_TO_UPDATE);
            }
            break;
            case UPDATA_LEVEL_UNABLE_DIRECT:
            {
                // snprintf(m_otaData.retInfo, OTA_RET_INFO_LEN, MASSAGE_INFO_CAN_NOT_UPDATE);
            }
            break;
            case UPDATA_LEVEL_ABLE:
                // snprintf(m_otaData.retInfo, OTA_RET_INFO_LEN,
                //          MASSAGE_INFO_CAN_UPDATE, m_otaData.nowVer, m_otaData.newVer);
                break;
            case UPDATA_LEVEL_RECOMMEND:
            {
                // snprintf(m_otaData.retInfo, OTA_RET_INFO_LEN,
                //          MASSAGE_INFO_RECONNED_UPDATE, m_otaData.nowVer, m_otaData.newVer);
            }
            break;
            case UPDATA_LEVEL_FORCE:
            {
                m_otaState = USER_OTA_STATE_RUNNING;
                // snprintf(m_otaData.retInfo, OTA_RET_INFO_LEN,
                //          MASSAGE_INFO_FORCE_UPDATE, m_otaData.nowVer, m_otaData.newVer);
                vTaskDelay(3000);
            }
            break;
            default:
                break;
            }
        }
        else
        {
            m_otaState = USER_OTA_STATE_REQUEST_ERR;
            // snprintf(m_otaData.retInfo, OTA_RET_INFO_LEN, MASSAGE_INFO_VER_CHECK_FAIL);
        }
    }

    if ((!(m_runState & RUN_STATE_WIFI_BIT_MASK)) &&
        USER_OTA_STATE_REQUEST_WAIT == m_otaState)
    {
        m_otaState = USER_OTA_STATE_REQUEST_ERR;
        m_otaData.updateResult = UPDATE_PROGRESS_NONE;
        // snprintf(m_otaData.retInfo, OTA_RET_INFO_LEN, MASSAGE_INFO_WIFI_UNCONNECTED);
    }
    return true;
}

bool Networker::ManageConfig(INFO_MANAGE_ACTION *action)
{
    // 配置文件重置
    switch (*action)
    {

    case INFO_MANAGE_ACTION_CONFIG_UTIL_RESET:
    {
        SH_UPDATE_CFG_OPERATE_LOCK(this->resetUtilConfig())
        *action = INFO_MANAGE_ACTION_CONFIG_UTIL_RESET_OK;
    }
    break;
    default:
        break;
    }

    return true;
}

bool Networker::MessageHandle(const char *from, const char *to,
                              CTRL_MESSAGE_TYPE type, void *message,
                              void *ext_info)
{
    switch (type)
    {
    case CTRL_MESSAGE_MQTT_SENT:
    {
        mqttPushTopic((char *)message, (char *)ext_info);
    }
    break;
    default:
        break;
    }

    return true;
}

void Networker::OtaHandle(USER_OTA_STATE *state)
{
    if (USER_OTA_STATE_IDLE == *state)
    {
        m_otaState = USER_OTA_STATE_IDLE;
        return;
    }

    if (USER_OTA_STATE_REQUEST == *state)
    {
        *state = USER_OTA_STATE_REQUEST_WAIT;
        m_otaState = USER_OTA_STATE_REQUEST_WAIT;
        // snprintf(m_otaData.retInfo, OTA_RET_INFO_LEN, MASSAGE_INFO_WAIT_AMOMENT);
        return;
    }

    if (USER_OTA_STATE_START == *state)
    {
        *state = USER_OTA_STATE_RUNNING;
        m_otaState = USER_OTA_STATE_RUNNING;
        // snprintf(m_otaData.retInfo, OTA_RET_INFO_LEN, MASSAGE_INFO_UPGRADING);
        return;
    }

    *state = m_otaState;
}

bool Networker::GetWeatherInfo(char *info)
{
    if (m_runState & RUN_STATE_WEATHER_BIT_MASK)
    {
    }
    return true;
}

bool Networker::UpdateConfig(void)
{
    if (m_utilConfig.wifiMode != m_utilConfigCache.wifiMode ||
        m_utilConfig.weatherEnable != m_utilConfigCache.weatherEnable ||
        // m_utilConfig.webServerEnable != m_utilConfigCache.webServerEnable ||
        m_utilConfig.ssid_1.compare(m_utilConfigCache.ssid_1) ||
        strcmp(m_utilConfig.password_1, m_utilConfigCache.password_1) ||
        m_utilConfig.ssid_2.compare(m_utilConfigCache.ssid_2) ||
        strcmp(m_utilConfig.password_2, m_utilConfigCache.password_2) ||
        strcmp(m_utilConfig.cityCode, m_utilConfigCache.cityCode) ||
        strcmp(m_utilConfig.apiKey, m_utilConfigCache.apiKey) ||
        strcmp(m_utilConfig.ignoredVer, m_utilConfigCache.ignoredVer) ||
        strcmp(m_utilConfig.snailID, m_utilConfigCache.snailID) ||
        m_utilConfig.weatherUpdataInterval != m_utilConfigCache.weatherUpdataInterval ||
        m_utilConfig.timeUpdataInterval != m_utilConfigCache.timeUpdataInterval ||
        m_utilConfig.noticeId != m_utilConfigCache.noticeId)
    {
        // bool restart = false;
        // if (m_utilConfig.webServerEnable != m_utilConfigCache.webServerEnable)
        // {
        //     restart = true;
        // }
        m_utilConfig = m_utilConfigCache;
        SH_UPDATE_CFG_OPERATE_LOCK(WriteConfig(&m_utilConfig));
        // if (restart)
        // {
        //     esp_restart();
        // }
    }

    return true;
}