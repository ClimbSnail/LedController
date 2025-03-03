#include "buzzer.h"
#include "config.h"
#include "sh_hal/hal.h"

#define BEEP_ON SH_HAL::SH_GPIO_LEVEL_HIGH
#define BEEP_OFF SH_HAL::SH_GPIO_LEVEL_LOW

volatile uint16_t Buzzer::m_count = 0;
uint16_t Buzzer::m_cycle_time = 10;
uint8_t Buzzer::m_pin_num = 0;
// uint8_t Buzzer::m_volume = 50;

/***************************************************
 * 初始化
 * buzzer_num为蜂鸣器的引脚
 * cycle_time为蜂鸣器控制的精度 ms
 */
Buzzer::Buzzer(uint8_t buzzer_num, uint16_t cycle_time)
{
    enable = false;
    m_count = 0;
    m_pin_num = buzzer_num;
    m_volume = 50;
    m_cycle_time = cycle_time != 0 ? cycle_time : 10;
}

bool Buzzer::Start()
{
    SH_HAL::gpioInit(m_pin_num, SH_HAL::SH_GPIO_MODE_OUTPUT,
                     SH_HAL::SH_GPIO_PULL_DOWN_ONLY, BEEP_OFF);

    setPwmParam(PWM_GROUP_0_FREQ << 7, PWM_GROUP_0_BIT_WIDTH,
                PWM_GROUP_0_UNIT_COUNT * PWM_GROUP_0_UNIT_VAL);

    enable = false; // 默认不使能蜂鸣器

    return true;
}

bool Buzzer::setPwmParam(uint32_t freq, uint8_t bitWidth, uint32_t unitCount)
{
    m_pwmFreq = freq;
    m_pwmBitWidth = bitWidth;
    m_pwmUnitCount = unitCount;

    set_beep_volume(m_volume);

    return true;
}

bool Buzzer::setAble(bool en)
{
    enable = en;
    return true;
}

void Buzzer::handler(void)
{
    if (0 < m_count)
    {
        --m_count;
    }
    else
    {
        // 定时时间到了 关闭蜂鸣器
#ifdef BEEP_CHANNEL
        SH_HAL::pwmEnable(m_pin_num, false, BEEP_OFF);
#else
        SH_HAL::gpioLevelSet(m_pin_num, BEEP_OFF);
#endif
    }
}

Buzzer::~Buzzer()
{
    enable = false;
    // 默认关闭蜂鸣器
#ifdef BEEP_CHANNEL
    SH_HAL::pwmEnable(m_pin_num, false, BEEP_OFF);
#else
    SH_HAL::gpioLevelSet(m_pin_num, BEEP_OFF);
#endif
}

/****************************************
 * 设置长鸣时间
 * time为长鸣时间 ms
 *****************************************/
void Buzzer::set_beep_time(uint16_t time)
{
    if (!enable)
    {
        return;
    }

    m_count = time / m_cycle_time;
    if (m_count == 0)
    {
        m_count = 1;
    }

    if (0 < m_count)
    {
        // 开始定时 开启蜂鸣器
#ifdef BEEP_CHANNEL
        // 连接pwm控制器
        SH_HAL::pwmEnable(m_pin_num, true, SH_HAL::SH_GPIO_LEVEL_UNDEF);
#else
        SH_HAL::gpioLevelSet(m_pin_num, BEEP_ON);
#endif
    }
}

/****************************************
 * 设置蜂鸣器音量
 * value取值范围 0~100
 *****************************************/
void Buzzer::set_beep_volume(uint8_t value)
{
    m_volume = value;

#ifdef BEEP_CHANNEL
    uint32_t freq = SH_HAL::pwmInit(m_pin_num, m_pwmFreq, m_pwmBitWidth);
    uint32_t duty = (uint32_t)(m_volume / 100.0 * m_pwmUnitCount);
    if (duty >= m_pwmUnitCount)
    {
        // 防止超限
        duty = m_pwmUnitCount;
    }
    
    SH_HAL::pwmSetDuty(m_pin_num, duty);
    SH_HAL::pwmEnable(m_pin_num, false, BEEP_OFF);
    
#else
    SH_HAL::gpioLevelSet(m_pin_num, BEEP_OFF);
#endif
}