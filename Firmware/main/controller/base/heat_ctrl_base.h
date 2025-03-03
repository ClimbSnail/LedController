#ifndef __HEAT_CONTROLLER_BASE_H
#define __HEAT_CONTROLLER_BASE_H

#include "sys/interface.h"
#include "controller_base.h"

#define TEMPERATURE_BUF_LEN 10
#define HP_TEMPERATURE_BUF_LEN 5

enum SHAKE_STATE : uint8_t
{
    SHAKE_STATE_UNKNOW = 0,
    SHAKE_STATE_WORK,
    SHAKE_STATE_WORK_TO_SLEEP,
    SHAKE_STATE_SLEEP,
    SHAKE_STATE_WAKE
};

#define DISPLAY_TEMP_STEP 100

enum DISPLAY_TEMP : unsigned char
{
    DISPLAY_TEMP_0 = 0,
    // DISPLAY_TEMP_50, // 显示温度为 50 的时候
    DISPLAY_TEMP_100,
    // DISPLAY_TEMP_150,
    DISPLAY_TEMP_200,
    // DISPLAY_TEMP_250,
    DISPLAY_TEMP_300,
    // DISPLAY_TEMP_350,
    DISPLAY_TEMP_400,
    // DISPLAY_TEMP_450,
    DISPLAY_TEMP_500,
    // DISPLAY_TEMP_550,
    DISPLAY_TEMP_600,
    DISPLAY_TEMP_MAX // DISPLAY_TEMP_MAX 不能小于2
};

class HeatControllerBase
{
protected:
    char m_name[16];         // 控制器名字
    SnailManager *m_manager; //
public:
    HeatControllerBase(const char *name, SnailManager *m_manager);
    // 析构函数
    virtual ~HeatControllerBase(){};
    // 控制器启动初始化 ps:只初始化赋值必要的变量，不进行任何引脚的初始化
    virtual bool Start() = 0;
    // 主处理程序
    virtual bool Process() = 0;
    // 控制器关闭
    virtual bool End() = 0;
    // 消息处理
    virtual bool MessageHandle(const char *from, const char *to,
                                CTRL_MESSAGE_TYPE type, void *message,
                                void *ext_info) = 0;
};

#endif