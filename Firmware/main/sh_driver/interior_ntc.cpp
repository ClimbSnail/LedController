#include "interior_ntc.h"
#include "common.h"
#include "cmath"
#include "sh_hal/hal.h"

#define NTC_R 10000.0  // 10k  SDNT1608X103/B:3380
#define R_PULL 10000.0 // 上拉10k
#define KA 273.15      // 绝对温度开尔

// NTC的温度公式
// Rt = R * EXP( B * (1/T1 - 1/T2) )
// R常温25C下阻值为 NTC_R
// B值为 3380
// Rt = NTC_R * EXP( NTC_B * (1/T1 - 1/(273.15+25)) )

/***************************************************
 * 初始化
 * channelPin 读取温度的ADC引脚
 *
 */
InteriorNTC::InteriorNTC()
{
    m_temperature = 25;
    m_ntcBValue = NTC_B_3950;
}

bool InteriorNTC::setBValue(double ntc_b_value)
{
    m_ntcBValue = ntc_b_value;
    return true;
}

bool InteriorNTC::init()
{
    SH_HAL::adcInit(NTC_ADC, SH_HAL::SH_ADC_ATTEN_11db);
    return true;
}

bool InteriorNTC::getTemperature(float *temperature)
{
    // ADC相关配置已经在"示波器"中初始化了
    // uint16_t vol = (uint16_t)oscilloscope->getChanne0();
    uint16_t vol = SH_HAL::adcRead(NTC_ADC, true);
    double vol_v = vol / 1000.0;
    // double curNTC_R = vol_v * R_T / (55 * (3.3 - vol_v) - vol_v);
    double curNTC_R = vol_v * R_PULL / (3.3 - vol_v);

    // SH_LOG("vol_v = %lf V\n", vol_v);
    // SH_LOG("curNTC_R = %lf R\n", curNTC_R);
    *temperature = 1 / (log(curNTC_R / NTC_R) / m_ntcBValue + (1 / (KA + 25))) - KA;

    // *temperature -= 10;

    if (*temperature > 50 || *temperature < -30 || isnan(*temperature))
    {
        // 异常
        *temperature = m_temperature;
        return false;
        SH_LOG("NTC value error, m_temperature forced assignment %.2f.\n", m_temperature);
    }
    else
    {
        m_temperature = *temperature;
    }
    return true;
}

InteriorNTC::~InteriorNTC()
{
}
