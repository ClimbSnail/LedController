#include "knobs.h"
#include "common.h"
#include "sh_hal/hal.h"
// #include "page/desktop/snail_ui.h"
// #include "../controller_base.h"

KeyInfo Knobs::m_key_info = {0, 0, KNOBS_STATE::KNOBS_STATE_IDLE};
uint8_t Knobs::m_pinA_num = 0;
uint8_t Knobs::m_pinB_num = 0;
uint8_t Knobs::m_pinSw_num = 0;
uint8_t Knobs::m_pinA_Status = 0;
uint8_t Knobs::m_pinB_Status = 0;
uint32_t Knobs::m_previousMillis = 0;
uint8_t Knobs::flag = 0;
bool Knobs::intellectRateFlag = 0;
bool Knobs::toneFlag = 0;

#define BEEP_TONE \
    if (toneFlag) \
    {             \
    }

/***********************************************************
 * 编码器类的初始化   注：在大多数情况下，
 * 只能使用引脚0、2、4、5、12、13、14、15和16
 *
 */
Knobs::Knobs(uint8_t pinA_num, uint8_t pinB_num, uint8_t pinSw_num)
{
    m_pinA_num = pinA_num;
    m_pinB_num = pinB_num;
    m_pinSw_num = pinSw_num;
    m_pinA_Status = 0;
    m_pinB_Status = 0;
    m_key_info.pulse_count = 0;
    m_key_info.switch_status = KNOBS_STATE::KNOBS_STATE_IDLE;
    m_key_info.switch_time = 0;
    flag = 0;
    intellectRateFlag = true;
    toneFlag = 0;

    m_direction = KNOBS_DIR_POS; // 默认正向
}

void Knobs::Start(void)
{
    SH_HAL::gpioInit(m_pinA_num, SH_HAL::SH_GPIO_MODE_INPUT,
                     SH_HAL::SH_GPIO_PULL_UP_ONLY, SH_HAL::SH_GPIO_LEVEL_UNDEF);
    SH_HAL::gpioInit(m_pinB_num, SH_HAL::SH_GPIO_MODE_INPUT,
                     SH_HAL::SH_GPIO_PULL_UP_ONLY, SH_HAL::SH_GPIO_LEVEL_UNDEF);
    SH_HAL::gpioInit(m_pinSw_num, SH_HAL::SH_GPIO_MODE_INPUT,
                     SH_HAL::SH_GPIO_PULL_UP_ONLY, SH_HAL::SH_GPIO_LEVEL_UNDEF);

    // 下降沿触发
    SH_HAL::gpioSetIntr(m_pinA_num, interruter_funcA,
                        SH_HAL::SH_GPIO_INTR_TYPE_ANYEDGE);
    // 按下与松开
    SH_HAL::gpioSetIntr(m_pinSw_num, interruter_funcSW_ON,
                        SH_HAL::SH_GPIO_INTR_TYPE_ANYEDGE);
}

void Knobs::setTone(bool flag)
{
    toneFlag = flag;
}

bool Knobs::setIntellectRateFlag(bool flag)
{
    intellectRateFlag = flag;
    return 0;
}

void Knobs::interruter_funcA(void *param)
{
    // 发生外部中断后执行的函数
    if (m_pinA_Status == 0 && SH_HAL::gpioLevelRead(m_pinA_num) == 0) // 第一次中断，并且A相是下降沿
    {
        flag = 0;
        if (SH_HAL::gpioLevelRead(m_pinB_num))
        {
            flag = 1; // 根据B相，设定标志
        }
        m_pinA_Status = 1; // 中断计数
    }
    if (m_pinA_Status && SH_HAL::gpioLevelRead(m_pinA_num)) // 第二次中断，并且A相是上升沿
    {
        m_pinB_Status = SH_HAL::gpioLevelRead(m_pinB_num);
        if (m_pinB_Status == 0 && flag == 1)
        {
            --(m_key_info.pulse_count);

            BEEP_TONE;
        }
        if (m_pinB_Status && flag == 0)
        {
            ++(m_key_info.pulse_count);

            BEEP_TONE;
        }
        m_pinA_Status = 0; // 中断计数复位，准备下一次
    }
}

void Knobs::interruter_funcSW_ON(void *param)
{
    // 发生外部中断后执行的函数
    int status = SH_HAL::gpioLevelRead(m_pinSw_num);
    if (0 == status) // 按下
    {
        // delay(1);   // 不能加延时，会发生中断异常
        status = SH_HAL::gpioLevelRead(m_pinSw_num);
        status = SH_HAL::gpioLevelRead(m_pinSw_num);
        status = SH_HAL::gpioLevelRead(m_pinSw_num);
        if (0 == status)
        {
            m_previousMillis = GET_SYS_MILLIS();
            m_key_info.switch_status = KNOBS_STATE::KNOBS_STATE_PRESS;
            m_key_info.switch_time = 0;
        }
    }
    else
    { // 松开
        // delay(1);   // 不能加延时，会发生中断异常
        status = SH_HAL::gpioLevelRead(m_pinSw_num);
        status = SH_HAL::gpioLevelRead(m_pinSw_num);
        status = SH_HAL::gpioLevelRead(m_pinSw_num);
        if (1 == status &&
            KNOBS_STATE::KNOBS_STATE_PRESS == m_key_info.switch_status)
        {
            m_key_info.switch_status = KNOBS_STATE::KNOBS_STATE_PRESS_LOST;
            m_key_info.switch_time = GET_SYS_MILLIS() - m_previousMillis;
            BEEP_TONE;
            // SH_LOG("KNOBS_STATE_PRESS_LOST");
        }
    }
}

bool Knobs::setDirection(KNOBS_DIR dir)
{
    m_direction = dir;
    return true;
}

// KeyInfo Knobs::get_data(void)
// {
//     KeyInfo ret_info = m_key_info;

//     m_key_info.pulse_count = 0;                               // clear
//     m_key_info.switch_status = KNOBS_STATE::KNOBS_STATE_IDLE; // clear
//     m_key_info.switch_time = 0;
//     return ret_info;
// }

KeyInfo Knobs::get_data(void)
{
    KeyInfo ret_info;

    int16_t pulse_count = m_key_info.pulse_count;
    m_key_info.pulse_count = 0; // clear

    if (ENABLE_STATE::ENABLE_STATE_OPEN == intellectRateFlag)
    {
        pulse_count = pulse_count * abs(pulse_count);
    }
    if (KNOBS_DIR_POS != m_direction)
    {
        pulse_count = -pulse_count;
    }

    KNOBS_STATE state = KNOBS_STATE::KNOBS_STATE_IDLE;
    if (KNOBS_STATE::KNOBS_STATE_IDLE != m_key_info.switch_status)
    {
        if (KNOBS_STATE::KNOBS_STATE_PRESS == m_key_info.switch_status &&
            GET_SYS_MILLIS() - m_previousMillis > 2000)
        {
            // SnailManager::heartbeat(); // 提示Manager接收到按钮动作

            m_key_info.switch_status = KNOBS_STATE::KNOBS_STATE_IDLE;
            m_key_info.switch_time = 0;
            state = KNOBS_STATE::KNOBS_STATE_PRESS_LOST_LONG_LONG;
        }
        else if (KNOBS_STATE::KNOBS_STATE_PRESS_LOST == m_key_info.switch_status &&
                 GET_SYS_MILLIS() - m_previousMillis > 800)
        {
            // SnailManager::heartbeat(); // 提示Manager接收到按钮动作

            m_key_info.switch_status = KNOBS_STATE::KNOBS_STATE_IDLE;
            m_key_info.switch_time = 0;
            state = KNOBS_STATE::KNOBS_STATE_PRESS_LOST_LONG;
            BEEP_TONE;
        }
        else if (KNOBS_STATE::KNOBS_STATE_PRESS_LOST == m_key_info.switch_status)
        {
            // SnailManager::heartbeat(); // 提示Manager接收到按钮动作

            m_key_info.switch_status = KNOBS_STATE::KNOBS_STATE_IDLE;
            m_key_info.switch_time = 0;
            state = KNOBS_STATE::KNOBS_STATE_PRESS_LOST;
        }
    }

    ret_info.pulse_count = pulse_count;
    ret_info.switch_status = state;
    return ret_info;
}

int16_t Knobs::getDiff(void)
{
    int16_t pulse_count = m_key_info.pulse_count;
    m_key_info.pulse_count = 0;

    if (pulse_count)
    {
        // SnailManager::heartbeat(); // 提示Manager接收到按钮动作
    }

    if (ENABLE_STATE::ENABLE_STATE_OPEN == intellectRateFlag)
    {
        pulse_count = pulse_count * abs(pulse_count);
        // pulse_count = pulse_count * pulse_count * pulse_count / 2;
    }

    if (KNOBS_DIR_POS == m_direction)
    {
        return pulse_count;
    }
    else
    {
        return -pulse_count;
    }
}

bool Knobs::getState(void)
{
    KNOBS_STATE state = KNOBS_STATE::KNOBS_STATE_IDLE;

    state = m_key_info.switch_status;

    if (KNOBS_STATE::KNOBS_STATE_IDLE == state)
    {
        return false;
    }

    if (KNOBS_STATE::KNOBS_STATE_PRESS == m_key_info.switch_status &&
        GET_SYS_MILLIS() - m_previousMillis > 1000)
    {
        // SnailManager::heartbeat(); // 提示Manager接收到按钮动作

        m_key_info.switch_status = KNOBS_STATE::KNOBS_STATE_IDLE;
        m_key_info.switch_time = 0;
        BEEP_TONE;
    }
    else if (KNOBS_STATE::KNOBS_STATE_PRESS_LOST == m_key_info.switch_status)
    {
        // SnailManager::heartbeat(); // 提示Manager接收到按钮动作

        m_key_info.switch_status = KNOBS_STATE::KNOBS_STATE_IDLE;
        return state == KNOBS_STATE::KNOBS_STATE_PRESS_LOST ? true : false;
    }
    return false;
}

Knobs::~Knobs()
{
}