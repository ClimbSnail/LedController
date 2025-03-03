#ifndef __KNOBS_H
#define __KNOBS_H

#include <stdint.h>

enum KNOBS_DIR : unsigned char
{
    KNOBS_DIR_POS = 0, // 正向
    KNOBS_DIR_NEG      // 反向
};

enum KNOBS_STATE : unsigned char
{
    KNOBS_STATE_IDLE = 0,             // 空闲状态
    KNOBS_STATE_PRESS,                // 按下
    KNOBS_STATE_PRESS_LOST,           // 短按松开
    KNOBS_STATE_PRESS_LOST_LONG,      // 长按松开
    KNOBS_STATE_PRESS_LOST_LONG_LONG, // 超长按松开
    KNOBS_STATE_MAX
};

struct KeyInfo
{
    int16_t pulse_count;
    // 按钮按下的时间
    uint16_t switch_time;
    KNOBS_STATE switch_status;
};

class Knobs
{
public:
    static KeyInfo m_key_info;
    KNOBS_DIR m_direction; // 旋钮的方向
    static uint8_t m_pinA_num;
    static uint8_t m_pinB_num;
    static uint8_t m_pinSw_num;
    static uint8_t m_pinA_Status;
    static uint8_t m_pinB_Status;
    static uint8_t flag;
    static bool intellectRateFlag; // 智能增速标志
    static bool toneFlag;
    static uint32_t m_previousMillis;

public:
    Knobs(uint8_t pinA_num, uint8_t pinB_num, uint8_t pinSw_num);
    ~Knobs();
    void Start(void);
    KeyInfo get_data(void);
    int16_t getDiff(void);                       // 获取旋转的数据
    bool setDirection(KNOBS_DIR dir);            // 设置方向
    void setTone(bool flag);                     // 设置是否开启提示音
    bool getState(void);                         // false松开 true按下
    static bool setIntellectRateFlag(bool flag); // 设置智能增速
    // void static interruter_funcA(void);
    void static interruter_funcA(void *param);
    void static interruter_funcSW_ON(void *param);
};

#endif