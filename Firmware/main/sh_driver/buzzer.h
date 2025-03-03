#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdint.h>

/**********************************
 * 蜂鸣器控制器
 */
class Buzzer
{
public:
    bool enable; // 使能
    static volatile uint16_t m_count;
    static uint16_t m_cycle_time;
    static uint8_t m_pin_num;
    uint8_t m_volume; // 音量的百分比
    uint8_t m_pwmBitWidth;
    uint32_t m_pwmFreq;
    uint32_t m_pwmUnitCount;

public:
    Buzzer(uint8_t buzzer_num, uint16_t cycle_time = 10);
    ~Buzzer();
    bool Start();
    bool setAble(bool en);
    void handler(void);
    void set_beep_time(uint16_t time);
    void set_beep_volume(uint8_t value); // 音量 0~100
    bool setPwmParam(uint32_t freq, uint8_t bitWidth, uint32_t unitCount);
};

#endif