#ifndef INTERFACE_H
#define INTERFACE_H

enum CTRL_MESSAGE_TYPE : unsigned char
{
    CTRL_MESSAGE_WIFI_CONN = 0, // 开启连接
    CTRL_MESSAGE_WIFI_AP,       // 开启AP事件
    CTRL_MESSAGE_WIFI_ALIVE,    // wifi开关的心跳维持
    CTRL_MESSAGE_WIFI_DISCONN,  // 连接断开
    CTRL_MESSAGE_UPDATE_TIME,
    CTRL_MESSAGE_GET_PARAM, // 获取参数
    CTRL_MESSAGE_SET_PARAM, // 设置参数
    CTRL_MESSAGE_READ_CFG,  // 向磁盘读取参数
    CTRL_MESSAGE_WRITE_CFG, // 向磁盘写入参数
    CTRL_MESSAGE_MQTT_DATA, // MQTT客户端收到消息
    CTRL_MESSAGE_MQTT_SENT, // MQTT客户端发送消息

    CTRL_MESSAGE_NONE
};

enum CTRL_RUN_TYPE : unsigned char
{
    CTRL_RUN_TYPE_REAL_TIME = 0, // 实时
    CTRL_RUN_TYPE_BACKGROUND,    // 后台

    CTRL_RUN_TYPE_NONE
};

enum CTRL_EVENT : unsigned char
{
    CTRL_EVENT_AIRHOT_CONNECT = 0,          // 连接
    CTRL_EVENT_AIRHOT_DISCONNECT,           // 断开
    CTRL_EVENT_AIRHOT_SLEEP,                // 风枪休眠
    CTRL_EVENT_AIRHOT_WAKE,                 // 风枪唤醒
    CTRL_EVENT_AIRHOT_TEMP_AlARM,           // 风枪超温提示
    CTRL_EVENT_AIRHOT_ABNORMAL_TERMINATION, // 风枪异常终止
    CTRL_EVENT_AITRHOT_ZERO_CHECK,          // 过零检测

    CTRL_EVENT_HEATPLATFORM_CONNECT,              // 连接
    CTRL_EVENT_HEATPLATFORM_DISCONNECT,           // 断开
    CTRL_EVENT_HEATPLATFORM_SLEEP,                // 加热台休眠
    CTRL_EVENT_HEATPLATFORM_WAKE,                 // 加热台唤醒
    CTRL_EVENT_HEATPLATFORM_TEMP_AlARM,           // 加热台超温提示
    CTRL_EVENT_HEATPLATFORM_ABNORMAL_TERMINATION, // 加热台异常终止
    CTRL_EVENT_HEATPLATFORM_CURVE_SAVE,           // 曲线保存成功

    CTRL_EVENT_SOLDER_CONNECT,              // 连接
    CTRL_EVENT_SOLDER_DISCONNECT,           // 断开
    CTRL_EVENT_SOLDER_DEEP_SLEEP,           // 烙铁深度休眠
    CTRL_EVENT_SOLDER_SLEEP,                // 烙铁休眠
    CTRL_EVENT_SOLDER_WAKE,                 // 烙铁唤醒
    CTRL_EVENT_SOLDER_TEMP_AlARM,           // 烙铁超温提示
    CTRL_EVENT_SOLDER_SHORT_CIRCUIT,        // 烙铁短路提示
    CTRL_EVENT_SOLDER_SHORT_CIRCUIT_CANCEL, // 烙铁短路取消提示
    CTRL_EVENT_SOLDER_RECO_CORE,            // 烙铁芯（手柄）类型识别

    CTRL_EVENT_ADJPWR_OPEN,        // 可调电源的触发打开
    CTRL_EVENT_ADJPWR_CLOSE,       // 可调电源的触发关闭
    CTRL_EVENT_ADJPWR_CUR_LIMIT,   // 可调电源的软件过流保护触发
    CTRL_EVENT_ADJPWR_CALIB_START, // 可调电源的校准开始
    CTRL_EVENT_ADJPWR_CALIB_END,   // 可调电源的校准结束
    CTRL_EVENT_ADJPWR_CALIB_FAIL,  // 可调电源的校准失败

    CTRL_EVENT_SPOTWELDER_OPEN,    //  点焊机打开
    CTRL_EVENT_SPOTWELDER_CLOSE,   //  点焊机关闭
    CTRL_EVENT_SPOTWELDER_TRIGGER, // 点焊机触发

    CTRL_EVENT_SIGNAL_OPEN,  // 信号发生器打开
    CTRL_EVENT_SIGNAL_CLOSE, // 信号发生器关闭

    CTRL_EVENT_NONE
};

#endif