#ifndef __INTERIOR_NTC_H
#define __INTERIOR_NTC_H

#define NTC_B_3380 3380.0
#define NTC_B_3950 3950.0

/**********************************
 * 内部NTC
 */
class InteriorNTC
{
public:
    float m_temperature; // 启动的室温
    double m_ntcBValue;  // NTC的B值

public:
    InteriorNTC();
    bool init();
    bool setBValue(double ntc_b_value);
    bool getTemperature(float *temperature);
    ~InteriorNTC();
};

#endif