#include "LedController.h"
#include "sh_hal/hal.h"
#include "config.h"
#include "common.h"

#define INDICATE_BRIGHTNESS 0.005f // 指示灯的亮度

LedController::LedController(const char *name, SnailManager *m_manager,
                             uint8_t pwmPin0, uint8_t pwmPin1,
                             uint8_t pwmPin2, uint8_t pwmPin3,
                             uint8_t fanPin)
    : ControllerBase(name, CTRL_TYPE_LED, m_manager)
{
    m_enableFlag = ENABLE_STATE_CLOSE;
    m_initFlag = false;

    m_pwmPin_0 = pwmPin0;
    m_pwmPin_1 = pwmPin1;
    m_pwmPin_2 = pwmPin2;
    m_pwmPin_3 = pwmPin3;

    m_fanPin = fanPin;

    m_opType = LED_OP_TYPE_BRIGHTNESS;
}

LedController::~LedController()
{
}

bool LedController::Start()
{
    // 读取配置文件
    this->ReadConfig(&m_utilConfig);
    m_utilConfig.colorRgb = 100;
    m_utilConfigCache = m_utilConfig;

    SH_HAL::gpioInit(m_pwmPin_0, SH_HAL::SH_GPIO_MODE_OUTPUT, SH_HAL::SH_GPIO_PULL_DOWN_ONLY);
    SH_HAL::gpioInit(m_pwmPin_1, SH_HAL::SH_GPIO_MODE_OUTPUT, SH_HAL::SH_GPIO_PULL_DOWN_ONLY);
    SH_HAL::gpioInit(m_pwmPin_2, SH_HAL::SH_GPIO_MODE_OUTPUT, SH_HAL::SH_GPIO_PULL_DOWN_ONLY);
    SH_HAL::gpioInit(m_pwmPin_3, SH_HAL::SH_GPIO_MODE_OUTPUT, SH_HAL::SH_GPIO_PULL_DOWN_ONLY);

    // LED pwm调光控制引脚 "无明显频闪"要求频率高于3125Hz
    SH_HAL::pwmInit(m_pwmPin_0, PWM_GROUP_0_FREQ, PWM_GROUP_0_BIT_WIDTH);
    SH_HAL::pwmInit(m_pwmPin_1, PWM_GROUP_0_FREQ, PWM_GROUP_0_BIT_WIDTH);
    SH_HAL::pwmInit(m_pwmPin_2, PWM_GROUP_1_FREQ, PWM_GROUP_1_BIT_WIDTH);
    SH_HAL::pwmInit(m_pwmPin_3, PWM_GROUP_1_FREQ, PWM_GROUP_1_BIT_WIDTH);

    this->Close();

    SwitchLedPanelType(m_utilConfigCache.ledPanelType);

    // 风扇
    SH_HAL::gpioInit(m_fanPin, SH_HAL::SH_GPIO_MODE_OUTPUT, SH_HAL::SH_GPIO_PULL_DOWN_ONLY);
    SH_HAL::pwmInit(m_fanPin, PWM_GROUP_2_FREQ, PWM_GROUP_2_BIT_WIDTH);
    SH_HAL::pwmSetDuty(m_fanPin, (uint32_t)(PWM_GROUP_2_UNIT_VAL * 0));

    // this->Application();

    m_enableFlag = ENABLE_STATE_OPEN;
    m_initFlag = true;

    return true;
}

bool LedController::Open()
{
    switch (m_utilConfigCache.ledMode)
    {
    case LED_MODE_CCT:
    {
        // m_utilConfigCache.colorTemperature = (LED_CT_LOW * x) + LED_CT_HIGH * (1 - x);
        float cctLowPwm = (LED_CT_HIGH - m_utilConfigCache.colorTemperature) * 1.0 / (LED_CT_HIGH - LED_CT_LOW);
        float cctHighPwm = 1.0 - cctLowPwm;
        SH_HAL::pwmSetDuty(m_pwmPin_0,
                           (uint32_t)(PWM_GROUP_0_UNIT_VAL * m_utilConfigCache.cctBrightness * cctHighPwm));
        SH_HAL::pwmSetDuty(m_pwmPin_1,
                           (uint32_t)(PWM_GROUP_0_UNIT_VAL * m_utilConfigCache.cctBrightness * cctLowPwm));

        // SH_LOG("PWM_GROUP_0_UNIT_VAL * m_utilConfigCache.cctBrightness * cctHighPwm = %.3lf !!!",
        //        (PWM_GROUP_0_UNIT_VAL * m_utilConfigCache.cctBrightness * cctHighPwm));
        // SH_LOG("PWM_GROUP_0_UNIT_VAL * m_utilConfigCache.cctBrightness * cctLowPwm = %.3lf !!!",
        //        (PWM_GROUP_0_UNIT_VAL * m_utilConfigCache.cctBrightness * cctLowPwm));
    }
    break;
    case LED_MODE_UV:
    {
        SH_HAL::pwmSetDuty(m_pwmPin_2, (uint32_t)(PWM_GROUP_1_UNIT_VAL * m_utilConfigCache.uvBrightness));
    }
    break;
    case LED_MODE_SINGLE:
    {
        SH_HAL::pwmSetDuty(m_pwmPin_0, (uint32_t)(PWM_GROUP_0_UNIT_VAL * m_utilConfigCache.singleBrightness));
    }
    break;
    case LED_MODE_RGB:
    {
        m_utilConfigCache.rgbRed = (m_utilConfigCache.colorRgb >> 16) / 255.0 * m_utilConfigCache.rgbBrightness;
        m_utilConfigCache.rgbGreen = (m_utilConfigCache.colorRgb & 0x0000FF00 >> 8) / 255.0 * m_utilConfigCache.rgbBrightness;
        m_utilConfigCache.rgbBlue = (m_utilConfigCache.colorRgb & 0x000000FF) / 255.0 * m_utilConfigCache.rgbBrightness;

        double rgbRed = (m_utilConfigCache.colorRgb >> 16) / 255.0 * m_utilConfigCache.rgbBrightness;
        double rgbGreen = (m_utilConfigCache.colorRgb & 0x0000FF00 >> 8) / 255.0 * m_utilConfigCache.rgbBrightness;
        double rgbBlue = (m_utilConfigCache.colorRgb & 0x000000FF) / 255.0 * m_utilConfigCache.rgbBrightness;

        SH_LOG("rgbRed = %lf\n", rgbRed);
        SH_LOG("rgbGreen = %lf\n", rgbGreen);
        SH_LOG("rgbBlue = %lf\n", rgbBlue);
        SH_HAL::pwmSetDuty(m_pwmPin_1, (uint32_t)(PWM_GROUP_0_UNIT_VAL * rgbRed));
        SH_HAL::pwmSetDuty(m_pwmPin_2, (uint32_t)(PWM_GROUP_1_UNIT_VAL * rgbGreen));
        SH_HAL::pwmSetDuty(m_pwmPin_3, (uint32_t)(PWM_GROUP_1_UNIT_VAL * rgbBlue));
    }
    break;
    default:
        break;
        return true;
    }
    return true;
}

bool LedController::Close()
{
    SH_HAL::pwmSetDuty(m_pwmPin_0, (uint32_t)(PWM_GROUP_0_UNIT_VAL * 0));
    SH_HAL::pwmSetDuty(m_pwmPin_1, (uint32_t)(PWM_GROUP_0_UNIT_VAL * 0));
    SH_HAL::pwmSetDuty(m_pwmPin_2, (uint32_t)(PWM_GROUP_1_UNIT_VAL * 0));
    SH_HAL::pwmSetDuty(m_pwmPin_3, (uint32_t)(PWM_GROUP_1_UNIT_VAL * 0));
    return true;
}

bool LedController::Process()
{
    static LED_PANEL_TYPE s_ledPanelType;
    static uint16_t s_colorTemperature;
    static uint32_t s_colorRgb;

    if (s_ledPanelType != m_utilConfigCache.ledPanelType)
    {
        s_ledPanelType = m_utilConfigCache.ledPanelType;
        SwitchLedPanelType(m_utilConfigCache.ledPanelType);

        this->Application();
    }
    if (s_colorTemperature != m_utilConfigCache.colorTemperature)
    {
        s_colorTemperature = m_utilConfigCache.colorTemperature;
        this->Application();
    }
    if (s_colorRgb != m_utilConfigCache.colorRgb)
    {
        SH_LOG("m_utilConfigCache.colorRgb = %lu\n", m_utilConfigCache.colorRgb);
        s_colorRgb = m_utilConfigCache.colorRgb;
        this->Application();
    }

    static float temperature = 25.0f;
    interior_ntc.getTemperature(&temperature);
    // SH_LOG("temperature = %lf C\n", temperature);
    if (temperature > m_utilConfigCache.coolTemp)
    {
        float per = (temperature - m_utilConfigCache.coolTemp) * 0.02f + 0.35f;
        per = constrain(per, 0.35f, 1.0f);
        SH_HAL::pwmSetDuty(m_fanPin, (uint32_t)(PWM_GROUP_2_UNIT_VAL * PWM_GROUP_2_UNIT_COUNT * per));
    }
    else if (temperature <= m_utilConfigCache.coolTemp - 10)
    {
        SH_HAL::pwmSetDuty(m_fanPin, (uint32_t)(0));
    }
    else if (temperature > 80.0f)
    {
        // 高温关机
        Shutdown();
    }

    return true;
}

bool LedController::End()
{
    // 结束
    this->UpdateConfig();
    this->Close();
    SH_HAL::pwmSetDuty(m_fanPin, (uint32_t)(PWM_GROUP_2_UNIT_VAL * 0));

    SH_HAL::pwmEnable(m_pwmPin_0, false, SH_HAL::SH_GPIO_LEVEL_LOW);
    SH_HAL::pwmEnable(m_pwmPin_1, false, SH_HAL::SH_GPIO_LEVEL_LOW);
    SH_HAL::pwmEnable(m_pwmPin_2, false, SH_HAL::SH_GPIO_LEVEL_LOW);
    SH_HAL::pwmEnable(m_pwmPin_3, false, SH_HAL::SH_GPIO_LEVEL_LOW);
    SH_HAL::pwmEnable(m_fanPin, false, SH_HAL::SH_GPIO_LEVEL_LOW);
    return true;
}

bool LedController::Application()
{
    // 应用
    this->Close();

    this->Open();

    return true;
}

bool LedController::IndicateLite()
{
    // 低功耗指示闪烁 5V的USB口大概只有120mA电流
    SH_HAL::pwmSetDuty(m_pwmPin_0, (uint32_t)(PWM_GROUP_0_UNIT_VAL * PWM_GROUP_0_UNIT_COUNT * INDICATE_BRIGHTNESS));
    SH_HAL::pwmSetDuty(m_pwmPin_1, (uint32_t)(PWM_GROUP_0_UNIT_VAL * PWM_GROUP_0_UNIT_COUNT * INDICATE_BRIGHTNESS));
    SH_HAL::pwmSetDuty(m_pwmPin_2, (uint32_t)(PWM_GROUP_1_UNIT_VAL * PWM_GROUP_1_UNIT_COUNT * INDICATE_BRIGHTNESS));
    SH_HAL::pwmSetDuty(m_pwmPin_3, (uint32_t)(PWM_GROUP_1_UNIT_VAL * PWM_GROUP_1_UNIT_COUNT * INDICATE_BRIGHTNESS));
    vTaskDelay(150);
    // vTaskDelay(2000);

    this->Close();
    vTaskDelay(150);

    return true;
}

bool LedController::Indicate()
{
    // 指示闪烁
    switch (m_utilConfigCache.ledMode)
    {
    case LED_MODE_CCT:
    {
        SH_HAL::pwmSetDuty(m_pwmPin_0, (uint32_t)(PWM_GROUP_0_UNIT_VAL * PWM_GROUP_0_UNIT_COUNT * INDICATE_BRIGHTNESS));
        SH_HAL::pwmSetDuty(m_pwmPin_1, (uint32_t)(PWM_GROUP_0_UNIT_VAL * PWM_GROUP_0_UNIT_COUNT * INDICATE_BRIGHTNESS));
    }
    break;
    case LED_MODE_UV:
    {
        SH_HAL::pwmSetDuty(m_pwmPin_2, (uint32_t)(PWM_GROUP_1_UNIT_VAL * PWM_GROUP_1_UNIT_COUNT * INDICATE_BRIGHTNESS));
    }
    break;
    case LED_MODE_SINGLE:
    {
        SH_HAL::pwmSetDuty(m_pwmPin_0, (uint32_t)(PWM_GROUP_0_UNIT_VAL * PWM_GROUP_0_UNIT_COUNT * INDICATE_BRIGHTNESS));
    }
    break;
    case LED_MODE_RGB:
    {
        SH_HAL::pwmSetDuty(m_pwmPin_1, (uint32_t)(PWM_GROUP_0_UNIT_VAL * PWM_GROUP_0_UNIT_COUNT * INDICATE_BRIGHTNESS));
        SH_HAL::pwmSetDuty(m_pwmPin_2, (uint32_t)(PWM_GROUP_1_UNIT_VAL * PWM_GROUP_1_UNIT_COUNT * INDICATE_BRIGHTNESS));
        SH_HAL::pwmSetDuty(m_pwmPin_3, (uint32_t)(PWM_GROUP_1_UNIT_VAL * PWM_GROUP_1_UNIT_COUNT * INDICATE_BRIGHTNESS));
    }
    break;
    default:
        break;
        return true;
    }
    vTaskDelay(300);

    this->Close();
    vTaskDelay(300);

    return true;
}

bool LedController::Reset()
{
    for (int i = 0; i < 10; ++i)
    {
        this->IndicateLite();
    }

    this->ResetUtilConfig();
    return true;
}

bool LedController::SwitchLedPanelType(uint8_t type)
{
    m_utilConfigCache.ledPanelType = (LED_PANEL_TYPE)(type % LED_PANEL_TYPE_MAX);
    switch (m_utilConfigCache.ledPanelType)
    {
    case LED_PANEL_TYPE_SINGLE_RGB:
    {
        if (LED_MODE_RGB != m_utilConfigCache.ledMode &&
            LED_MODE_SINGLE != m_utilConfigCache.ledMode)
        {
            m_utilConfigCache.ledMode = LED_MODE_SINGLE;
        }
    }
    break;
    case LED_PANEL_TYPE_CCT_UV:
    {
        if (LED_MODE_UV != m_utilConfigCache.ledMode &&
            LED_MODE_CCT != m_utilConfigCache.ledMode)
        {
            m_utilConfigCache.ledMode = LED_MODE_CCT;
        }
    }
    break;
    default:
        break;
    }
    SH_LOG("ledPanelType = %u !!!", m_utilConfigCache.ledPanelType);
    return true;
}

bool LedController::SwitchLedMode(uint8_t mode)
{
    // m_utilConfigCache.ledMode = (LED_MODE)(mode % LED_MODE_MAX);

    switch (m_utilConfigCache.ledPanelType)
    {
    case LED_PANEL_TYPE_SINGLE_RGB:
    {
        if (LED_MODE_RGB == m_utilConfigCache.ledMode)
        {
            m_utilConfigCache.ledMode = LED_MODE_SINGLE;
        }
        else
        {
            m_utilConfigCache.ledMode = LED_MODE_RGB;
        }
    }
    break;
    case LED_PANEL_TYPE_CCT_UV:
    {
        if (LED_MODE_UV == m_utilConfigCache.ledMode)
        {
            m_utilConfigCache.ledMode = LED_MODE_CCT;
        }
        else
        {
            m_utilConfigCache.ledMode = LED_MODE_UV;
        }
    }
    break;
    default:
        break;
    }
    SH_LOG("ledMode = %u !!!", m_utilConfigCache.ledMode);

    return true;
}

bool LedController::SwitchOpType(void)
{
    if (LED_MODE_UV == m_utilConfigCache.ledMode)
    {
        // UV模式无其他操作
        m_opType = LED_OP_TYPE_BRIGHTNESS;
        return true;
    }

    m_opType = (LED_OP_TYPE)((m_opType + 1) % LED_OP_TYPE_MAX);
    SH_LOG("m_opType = %u !!!", m_opType);

    Indicate();
    if (LED_OP_TYPE_BRIGHTNESS == m_opType)
    {
        // 只闪烁一次
        return true;
    }
    Indicate();

    return true;
}
bool LedController::ResetBrightness(void)
{
    m_utilConfigCache.cctBrightness = 5;
    m_utilConfigCache.uvBrightness = 5;
    m_utilConfigCache.singleBrightness = 5;
    m_utilConfigCache.rgbBrightness = 5;
    this->UpdateConfig();
    return true;
}

bool LedController::AddBrightness(int16_t brightness)
{
    switch (m_utilConfigCache.ledMode)
    {
    case LED_MODE_CCT:
    {
        m_utilConfigCache.cctBrightness = constrain(m_utilConfigCache.cctBrightness + brightness,
                                                    5, PWM_GROUP_0_UNIT_COUNT);
        SH_LOG("setBrightness = %d !!!", m_utilConfigCache.cctBrightness);
    }
    break;
    case LED_MODE_UV:
    {
        m_utilConfigCache.uvBrightness = constrain(m_utilConfigCache.uvBrightness + brightness,
                                                   5, PWM_GROUP_0_UNIT_COUNT);
        SH_LOG("setBrightness = %d !!!", m_utilConfigCache.uvBrightness);
    }
    break;
    case LED_MODE_SINGLE:
    {
        m_utilConfigCache.singleBrightness = constrain(m_utilConfigCache.singleBrightness + brightness,
                                                       5, PWM_GROUP_0_UNIT_COUNT);
        SH_LOG("setBrightness = %d !!!", m_utilConfigCache.singleBrightness);
    }
    break;
    case LED_MODE_RGB:
    {
        m_utilConfigCache.rgbBrightness = constrain(m_utilConfigCache.rgbBrightness + brightness,
                                                    5, PWM_GROUP_0_UNIT_COUNT);
        SH_LOG("setBrightness = %d !!!", m_utilConfigCache.rgbBrightness);
    }
    break;
    default:
        break;
        return true;
    }

    return true;
}

bool LedController::AddSecondFuncValue(int16_t value)
{
    switch (m_utilConfigCache.ledMode)
    {
    case LED_MODE_CCT:
    {
        int16_t colorTemperature = (int16_t)m_utilConfigCache.colorTemperature + 30 * value;
        SH_LOG("colorTemperature = %d !!!", colorTemperature);
        m_utilConfigCache.colorTemperature = constrain(colorTemperature, LED_CT_LOW, LED_CT_HIGH);
    }
    break;
    case LED_MODE_UV:
    {
        // 无第二操作模式
    }
    break;
    case LED_MODE_SINGLE:
    {
        // 无第二操作模式
    }
    break;
    case LED_MODE_RGB:
    {
        // 待添加
    }
    break;
    default:
        break;
    }
    return true;
}

bool LedController::Operate(KNOBS_STATE knobsState, int16_t knobsValue)
{
    // 关机状态下
    if (ENABLE_STATE_CLOSE == m_enableFlag)
    {
        if (KNOBS_STATE_IDLE != knobsState || 0 != knobsValue)
        {
            m_enableFlag = ENABLE_STATE_CLOSE;
            this->Application();
            return true;
        }
    }

    if (KNOBS_STATE_PRESS_LOST_LONG == knobsState)
    {
        SwitchLedMode(m_utilConfigCache.ledMode + 1);
        this->Application();
    }
    else if (KNOBS_STATE_PRESS_LOST == knobsState)
    {
        SwitchOpType();
        this->Application();
    }

    if (0 != knobsValue)
    {
        if (LED_OP_TYPE_BRIGHTNESS == m_opType)
        {
            AddBrightness(5 * knobsValue);
            this->Application();
        }
        else if (LED_OP_TYPE_SECOND_FUNC == m_opType)
        {
            AddSecondFuncValue(knobsValue);
            this->Application();
        }
    }

    return true;
}

bool LedController::UpdateConfig(void)
{
    if (m_utilConfig.ledPanelType != m_utilConfigCache.ledPanelType ||
        m_utilConfig.ledMode != m_utilConfigCache.ledMode ||
        m_utilConfig.cctBrightness != m_utilConfigCache.cctBrightness ||
        m_utilConfig.colorTemperature != m_utilConfigCache.colorTemperature ||
        m_utilConfig.uvBrightness != m_utilConfigCache.uvBrightness ||
        m_utilConfig.rgbBrightness != m_utilConfigCache.rgbBrightness ||
        m_utilConfig.rgbRed != m_utilConfigCache.rgbRed ||
        m_utilConfig.rgbGreen != m_utilConfigCache.rgbGreen ||
        m_utilConfig.rgbBlue != m_utilConfigCache.rgbBlue ||
        m_utilConfig.coolTemp != m_utilConfigCache.coolTemp)
    {
        m_utilConfig = m_utilConfigCache;
        SH_UPDATE_CFG_OPERATE_LOCK(WriteConfig(&m_utilConfig));
    }
    return true;
}

// 消息处理
bool LedController::MessageHandle(const char *from, const char *to,
                                  CTRL_MESSAGE_TYPE type, void *message,
                                  void *ext_info)
{
    return true;
}