#ifndef __LED_CONTROLLER_H
#define __LED_CONTROLLER_H

#include <stdint.h>
#include "sh_driver/knobs.h"
#include "controller_base.h"

#define LED_CT_LOW 2700  // 最低色温K
#define LED_CT_HIGH 6500 // 最高色温K

// LED灯板类型
enum LED_PANEL_TYPE : unsigned char
{
    LED_PANEL_TYPE_SINGLE_RGB = 0, // 单色光+RGB
    LED_PANEL_TYPE_CCT_UV = 1,     // 可调色温+UV
    LED_PANEL_TYPE_MAX
};

// LED模式
enum LED_MODE : unsigned char
{
    LED_MODE_CCT = 0, // 色温模式
    LED_MODE_UV,      // UV模式
    LED_MODE_SINGLE,  // 单色模式
    LED_MODE_RGB,     // RGB模式
    LED_MODE_MAX
};

// 操作类型
enum LED_OP_TYPE : unsigned char
{
    LED_OP_TYPE_BRIGHTNESS = 0, // 亮度设置
    LED_OP_TYPE_SECOND_FUNC,    // 第二功能
    LED_OP_TYPE_MAX
};

// LED设置参数
struct LedUtilConfig
{
    LED_PANEL_TYPE ledPanelType; // LED灯板类型
    LED_MODE ledMode;            // 当前运行的LED模式

    uint16_t cctBrightness;    // 色温模式下的亮度 0~1000
    uint16_t colorTemperature; // 色温值

    uint16_t uvBrightness; // UV模式亮度 0~1000

    uint16_t singleBrightness; // 模式亮度 0~1000

    uint16_t rgbBrightness; // RGB模式亮度 0~1000
    uint32_t colorRgb;      // RGB
    uint8_t rgbRed;         // RGB模式红色分量 0~255
    uint8_t rgbGreen;       // RGB模式绿色分量 0~255
    uint8_t rgbBlue;        // RGB模式蓝色分量 0~255

    uint8_t coolTemp; // 散热开启的温度
};

/**********************************
 * LED控制器
 */

class LedController : public ControllerBase
{

private:
    uint8_t m_pwmPin_0; // pwm引脚
    uint8_t m_pwmPin_1; // pwm引脚
    uint8_t m_pwmPin_2; // pwm引脚
    uint8_t m_pwmPin_3; // pwm引脚

    uint8_t m_fanPin; // pwm引脚

    LED_OP_TYPE m_opType; // 操作类型

public:
    LedUtilConfig m_utilConfig;      // 通用配置信息
    LedUtilConfig m_utilConfigCache; // 运行时的通用配置信息

public:
    LedController(const char *name, SnailManager *m_manager,
                  uint8_t pwmPin0, uint8_t pwmPin1,
                  uint8_t pwmPin2, uint8_t pwmPin3,
                  uint8_t fanPin);
    ~LedController();
    bool Start();
    bool Open();
    bool Close();
    bool Process();
    bool End();
    // 消息处理
    bool MessageHandle(const char *from, const char *to,
                       CTRL_MESSAGE_TYPE type, void *message,
                       void *ext_info);
    bool Reset();
    bool IndicateLite();
    bool Indicate();
    bool SwitchLedPanelType(uint8_t type);
    bool SwitchLedMode(uint8_t mode);
    bool SwitchOpType(void);
    bool ResetBrightness(void); // 复位亮度
    bool AddBrightness(int16_t brightness);
    bool AddSecondFuncValue(int16_t value);
    bool Operate(KNOBS_STATE knobsState, int16_t knobsValue);
    void WriteConfig(LedUtilConfig *cfg);
    void ReadConfig(LedUtilConfig *cfg);
    bool ResetUtilConfig(void);
    bool UpdateConfig(void);
    bool Application();
};

#endif // __LED_CONTROLLER_H