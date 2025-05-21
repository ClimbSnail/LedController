#include "common.h"
#include "LedController.h"
#include "string.h"

// 持久化配置
#define LED_UTIL_CONFIG_PATH (ROOT_DIR "/led_util_v136_0.cfg")

void LedController::WriteConfig(LedUtilConfig *cfg)
{
    char tmp[16];
    // 将配置数据保存在文件中（持久化）
    string w_data;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->ledPanelType);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->ledMode);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->cctBrightness);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->colorTemperature);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->uvBrightness);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->singleBrightness);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->rgbBrightness);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%lu\n", cfg->colorRgb);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->rgbRed);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->rgbGreen);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->rgbBlue);
    w_data += tmp;

    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%u\n", cfg->coolTemp);
    w_data += tmp;

    g_flashCfg.writeFile(LED_UTIL_CONFIG_PATH, w_data.c_str());
}

void LedController::ReadConfig(LedUtilConfig *cfg)
{
    // 如果有需要持久化配置文件 可以调用此函数将数据存在flash中
    // 配置文件名最好以Controller名为开头 以".cfg"结尾，以免多个Controller读取混乱
    char *info = dataCache;
    uint16_t size = 0;

    size = g_flashCfg.readFile(LED_UTIL_CONFIG_PATH, (uint8_t *)info);

    info[size] = 0;
    if (size == 0)
    {
        // 默认值
        // LED_PANEL_TYPE_SINGLE_RGB LED_PANEL_TYPE_CCT_UV
        cfg->ledPanelType = LED_PANEL_TYPE_CCT_UV;
        cfg->ledMode = LED_MODE_CCT;
        cfg->cctBrightness = 20;
        cfg->colorTemperature = 4000; // 5000K
        cfg->uvBrightness = 20;
        cfg->singleBrightness = 20;
        cfg->rgbBrightness = 20;
        cfg->colorRgb = 16734288;
        cfg->rgbRed = 127;
        cfg->rgbGreen = 127;
        cfg->rgbBlue = 127;
        cfg->coolTemp = 45;

        WriteConfig(cfg);
    }
    else
    {
        int index = 0;
        // 解析数据
        char *param[12] = {0};
        analyseParam(info, 12, param);
        cfg->ledPanelType = (LED_PANEL_TYPE)atoi(param[index++]);
        cfg->ledMode = (LED_MODE)atoi(param[index++]);

        cfg->cctBrightness = atoi(param[index++]);
        cfg->colorTemperature = atoi(param[index++]);

        cfg->uvBrightness = atoi(param[index++]);
        cfg->singleBrightness = atoi(param[index++]);

        cfg->rgbBrightness = atoi(param[index++]);
        cfg->colorRgb = atol(param[index++]);
        cfg->rgbRed = atoi(param[index++]);
        cfg->rgbGreen = atoi(param[index++]);
        cfg->rgbBlue = atoi(param[index++]);
        cfg->coolTemp = atoi(param[index++]);
    }
}

// 重置通用配置
bool LedController::ResetUtilConfig(void)
{
    g_flashCfg.deleteFile(LED_UTIL_CONFIG_PATH);
    LedUtilConfig cfg;
    this->ReadConfig(&cfg);
    return true;
}
