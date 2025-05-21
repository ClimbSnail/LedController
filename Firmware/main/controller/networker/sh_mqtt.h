#ifndef _SH_MQTT_H
#define _SH_MQTT_H

enum MQTT_MSG_TYPE : unsigned char
{
    MQTT_MSG_TYPE_UNKNOWN = 0, // 未知消息
    MQTT_MSG_TYPE_CLOSE_LED,   // 关闭LED灯
    MQTT_MSG_TYPE_OPEN_LED,    // 打开LED灯
    MQTT_MSG_TYPE_MAX
};

// mqtt消息队列
extern QueueHandle_t mqttMsgQue;

void mqttStart(const char *device_id,      // 控制器的机器码
               const char *snail_device_id // 要联动的焊台机器码
);
void mqttPushTopic(const char *topic, const char *payload);

#endif // _SH_MQTT_H
