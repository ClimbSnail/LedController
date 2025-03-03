
#include "common.h"
#include "networker.h"
#include "networker_data.h"
#include "string.h"

// 持久化配置
#define NETWORKER_CONFIG_PATH (ROOT_DIR "/networker_v13_0.cfg")
#define NETWORKER_WEATHER_CONFIG_PATH (ROOT_DIR "/networkerWea_v13_0.cfg")

void Networker::WriteConfig(NetworkerConfig *cfg)
{
    char tmp[64];
    // 将配置数据保存在文件中（持久化）
    string w_data;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->wifiMode);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->weatherEnable);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->webServerEnable);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->mqttEnable);
    w_data += tmp;

    memset(tmp, 0, sizeof(cfg->ssid_0));
    snprintf(tmp, sizeof(cfg->ssid_0), "%s\n", cfg->ssid_0);
    w_data += tmp;

    memset(tmp, 0, sizeof(cfg->password_0));
    snprintf(tmp, sizeof(cfg->password_0), "%s\n", cfg->password_0);
    w_data += tmp;

    w_data = w_data + cfg->ssid_1 + "\n";

    memset(tmp, 0, sizeof(cfg->password_1));
    snprintf(tmp, sizeof(cfg->password_1), "%s\n", cfg->password_1);
    w_data += tmp;

    w_data = w_data + cfg->ssid_2 + "\n";

    memset(tmp, 0, sizeof(cfg->password_2));
    snprintf(tmp, sizeof(cfg->password_2), "%s\n", cfg->password_2);
    w_data += tmp;

    memset(tmp, 0, sizeof(cfg->cityCode));
    snprintf(tmp, sizeof(cfg->cityCode), "%s\n", cfg->cityCode);
    w_data += tmp;

    memset(tmp, 0, sizeof(cfg->apiKey));
    snprintf(tmp, sizeof(cfg->apiKey), "%s\n", cfg->apiKey);
    w_data += tmp;

    memset(tmp, 0, sizeof(cfg->ignoredVer));
    snprintf(tmp, sizeof(cfg->ignoredVer), "%s\n", cfg->ignoredVer);
    w_data += tmp;

    memset(tmp, 0, sizeof(cfg->snailID));
    snprintf(tmp, sizeof(cfg->snailID), "%s\n", cfg->snailID);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%lu\n", cfg->weatherUpdataInterval);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%lu\n", cfg->timeUpdataInterval);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->noticeId);
    w_data += tmp;

    g_flashCfg.writeFile(NETWORKER_CONFIG_PATH, w_data.c_str());
}

void Networker::ReadConfig(NetworkerConfig *cfg)
{
    // 如果有需要持久化配置文件 可以调用此函数将数据存在flash中
    // 配置文件名最好以Controller名为开头 以".cfg"结尾，以免多个Controller读取混乱
    char *info = dataCache;
    uint16_t size = 0;
    size = g_flashCfg.readFile(NETWORKER_CONFIG_PATH, (uint8_t *)info);

    info[size] = 0;
    if (size == 0)
    {
        // 默认值
        cfg->wifiMode = SH_WIFI_MODE_STA_AP;
        cfg->weatherEnable = ENABLE_STATE_OPEN;
        cfg->webServerEnable = ENABLE_STATE_OPEN;
        cfg->mqttEnable = ENABLE_STATE_CLOSE;
        snprintf(cfg->ssid_0, sizeof(cfg->ssid_0), "%s", "Snail");
        snprintf(cfg->password_0, sizeof(cfg->password_0), "%s", "12345678");
        cfg->ssid_1 = "Wifi_1名称";
        snprintf(cfg->password_1, sizeof(cfg->password_1), "%s", "Wifi_1密码");
        cfg->ssid_2 = "Wifi_2名称";
        snprintf(cfg->password_2, sizeof(cfg->password_2), "%s", "Wifi_2密码");
        snprintf(cfg->cityCode, sizeof(cfg->cityCode), "%s", "北京"); // "110000" "北京"
        snprintf(cfg->apiKey, sizeof(cfg->apiKey), "%s", "");
        snprintf(cfg->ignoredVer, sizeof(cfg->ignoredVer), "%s", LED_CONTROLLER_VERSION);
        snprintf(cfg->snailID, sizeof(cfg->snailID), "%s", "焊台机器码");

        cfg->weatherUpdataInterval = 1800000; // 天气更新的时间间隔900000(900s)
        cfg->timeUpdataInterval = 1800000;    // 日期时钟更新的时间间隔900000(900s)
        cfg->noticeId = 0;
        WriteConfig(cfg);
    }
    else
    {
        int index = 0;
        // 解析数据
        char *param[17] = {0};
        analyseParam(info, 17, param);
        cfg->wifiMode = (SH_WIFI_MODE)atoi(param[index++]);
        cfg->weatherEnable = (ENABLE_STATE)atoi(param[index++]);
        cfg->webServerEnable = (ENABLE_STATE)atoi(param[index++]);
        cfg->mqttEnable = (ENABLE_STATE)atoi(param[index++]);
        snprintf(cfg->ssid_0, sizeof(cfg->ssid_0), "%s", param[index++]);
        snprintf(cfg->password_0, sizeof(cfg->password_0), "%s", param[index++]);
        cfg->ssid_1 = param[index++];
        snprintf(cfg->password_1, sizeof(cfg->password_1), "%s", param[index++]);
        cfg->ssid_2 = param[index++];
        snprintf(cfg->password_2, sizeof(cfg->password_2), "%s", param[index++]);
        snprintf(cfg->cityCode, sizeof(cfg->cityCode), "%s", param[index++]);
        snprintf(cfg->apiKey, sizeof(cfg->apiKey), "%s", param[index++]);
        snprintf(cfg->ignoredVer, sizeof(cfg->ignoredVer), "%s", param[index++]);
        snprintf(cfg->snailID, sizeof(cfg->snailID), "%s", param[index++]);
        cfg->weatherUpdataInterval = atol(param[index++]);
        cfg->timeUpdataInterval = atol(param[index++]);
        cfg->noticeId = atoi(param[index++]);
    }
}

void Networker::WriteConfig(WeatherData *cfg)
{
    char tmp[64];
    // 将配置数据保存在文件中（持久化）
    string w_data;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%d\n", cfg->weatherCode);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%d\n", cfg->temperature);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%d\n", cfg->humidity);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%d\n", cfg->maxTemp);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%d\n", cfg->minTemp);
    w_data += tmp;

    memset(tmp, 0, sizeof(cfg->cityname));
    snprintf(tmp, sizeof(cfg->cityname), "%s\n", cfg->cityname);
    w_data += tmp;

    memset(tmp, 0, sizeof(cfg->windDir));
    snprintf(tmp, sizeof(cfg->windDir), "%s\n", cfg->windDir);
    w_data += tmp;

    memset(tmp, 0, sizeof(cfg->windpower));
    snprintf(tmp, sizeof(cfg->windpower), "%s\n", cfg->windpower);
    w_data += tmp;

    memset(tmp, 0, sizeof(cfg->weather));
    snprintf(tmp, sizeof(cfg->weather), "%s\n", cfg->weather);
    w_data += tmp;

    g_flashCfg.writeFile(NETWORKER_WEATHER_CONFIG_PATH, w_data.c_str());
}

void Networker::ReadConfig(WeatherData *cfg)
{
    // 如果有需要持久化配置文件 可以调用此函数将数据存在flash中
    // 配置文件名最好以Controller名为开头 以".cfg"结尾，以免多个Controller读取混乱
    char *info = dataCache;
    uint16_t size = 0;
    size = g_flashCfg.readFile(NETWORKER_WEATHER_CONFIG_PATH, (uint8_t *)info);

    info[size] = 0;
    if (size == 0)
    {
        // 默认值
        cfg->weatherCode = 0;
        cfg->temperature = 25;
        cfg->humidity = 80;
        cfg->maxTemp = 30;
        cfg->minTemp = 20;
        snprintf(cfg->cityname, sizeof(cfg->cityname), "%s", "北京市");
        snprintf(cfg->windDir, sizeof(cfg->windDir), "%s", "东南");
        snprintf(cfg->windpower, sizeof(cfg->windpower), "%s", "4");
        snprintf(cfg->weather, sizeof(cfg->weather), "%s", "晴");
        WriteConfig(cfg);
    }
    else
    {
        int index = 0;
        // 解析数据
        char *param[9] = {0};
        analyseParam(info, 9, param);
        cfg->weatherCode = atoi(param[index++]);
        cfg->temperature = atoi(param[index++]);
        cfg->humidity = atoi(param[index++]);
        cfg->maxTemp = atoi(param[index++]);
        cfg->minTemp = atoi(param[index++]);
        snprintf(cfg->cityname, sizeof(cfg->cityname), "%s", param[index++]);
        snprintf(cfg->windDir, sizeof(cfg->windDir), "%s", param[index++]);
        snprintf(cfg->windpower, sizeof(cfg->windpower), "%s", param[index++]);
        snprintf(cfg->weather, sizeof(cfg->weather), "%s", param[index++]);
    }
}

// 重置通用配置
bool Networker::resetUtilConfig(void)
{
    g_flashCfg.deleteFile(NETWORKER_CONFIG_PATH);
    NetworkerConfig cfg;
    this->ReadConfig(&cfg);
    return true;
}