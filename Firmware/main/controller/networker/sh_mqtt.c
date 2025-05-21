/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "sh_mqtt.h"
#include "config.h"
// #include "common.h"

unsigned char mqttMsgType = MQTT_MSG_TYPE_UNKNOWN;
QueueHandle_t mqttMsgQue;

static const char *TAG = "mqtt_example";
static bool g_mqtt_connected = false;
static char g_client_id[SNAIL_MQTT_DEVIVCE_ID_SIZE] = {0};                   // 控制器的ClientID
static char g_top_head[SNAIL_MQTT_TOPIC_HEAD_SIZE] = {0};                    // 消息头 目前刚好23字节
static char g_last_will_top[SNAIL_MQTT_LAST_WILL_TOP_SIZE] = {0};            // 异常断线后发布的消息
static char g_snail_subscribe_topic[SNAIL_MQTT_NORMAL_TOPIC_MAX_SIZE] = {0}; // 订阅的焊台topic
static esp_mqtt_client_handle_t g_client = NULL;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        // 客户端成功连接
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        g_mqtt_connected = true;
        // 主动发布消息
        char startup_topic[32] = {0};
        snprintf(startup_topic, 32, "%s%s", g_top_head, SNAIL_MQTT_TOPIC_SYS);
        msg_id = esp_mqtt_client_publish(client, startup_topic, SNAIL_MQTT_PAYLOAD_STARTUP,
                                         strlen(SNAIL_MQTT_PAYLOAD_STARTUP), 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        // 订阅主题
        msg_id = esp_mqtt_client_subscribe(client, SNAIL_MQTT_TOPIC_SYS_MANAGER, 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // 订阅主题
        msg_id = esp_mqtt_client_subscribe(client, g_snail_subscribe_topic, 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        // 退订主题
        // msg_id = esp_mqtt_client_unsubscribe(client, SNAIL_MQTT_TOPIC_SOLDER);
        // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        // 客户端断开连接
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        g_mqtt_connected = false;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        // 服务器已确认订阅主题
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        // 服务器已确认取消订阅主题
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        // 服务器已确认发布消息
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        // 收到MQTT消息
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        char topic[SNAIL_MQTT_NORMAL_TOPIC_MAX_SIZE] = {0};
        char data[SNAIL_MQTT_NORMAL_PAYLOAD_MAX_SIZE] = {0};
        strncpy(topic, event->topic, event->topic_len);
        topic[event->topic_len] = 0;
        strncpy(data, event->data, event->data_len);
        data[event->data_len] = 0;

        if (0 > strcmp(g_snail_subscribe_topic, topic))
        {
            mqttMsgType = MQTT_MSG_TYPE_UNKNOWN;
            if (0 == strcmp(SNAIL_MQTT_PAYLOAD_STARTUP, data))
            {
                mqttMsgType = MQTT_MSG_TYPE_OPEN_LED;
            }
            else if (0 == strcmp(SNAIL_MQTT_PAYLOAD_OFFLINE, data))
            {
                mqttMsgType = MQTT_MSG_TYPE_CLOSE_LED;
            }
            else if (0 == strcmp(SNAIL_MQTT_PAYLOAD_WAKE, data))
            {
                mqttMsgType = MQTT_MSG_TYPE_OPEN_LED;
            }
            else if (0 == strcmp(SNAIL_MQTT_PAYLOAD_SLEEP, data))
            {
                mqttMsgType = MQTT_MSG_TYPE_CLOSE_LED;
            }

            if (MQTT_MSG_TYPE_UNKNOWN != mqttMsgType)
            {
                xQueueSend(mqttMsgQue, &mqttMsgType, NULL);
            }
        }
        else if (0 < strcmp(g_snail_subscribe_topic, event->topic))
        {
            ESP_LOGI(TAG, "0 < strcmp(g_snail_subscribe_topic, event->topic)");
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqttStart(const char *device_id,      // 控制器的机器码
               const char *snail_device_id // 要联动的焊台机器码
)
{
    snprintf(g_client_id, SNAIL_MQTT_DEVIVCE_ID_SIZE, "Led_%s", device_id);
    snprintf(g_top_head, SNAIL_MQTT_TOPIC_HEAD_SIZE, "/led/%s", device_id);
    snprintf(g_last_will_top, SNAIL_MQTT_LAST_WILL_TOP_SIZE, "%s%s",
             g_top_head, SNAIL_MQTT_TOPIC_SYS);
    snprintf(g_snail_subscribe_topic, SNAIL_MQTT_NORMAL_TOPIC_MAX_SIZE,
             SNAIL_MQTT_TOPIC_SNAIL, snail_device_id);

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = SNAIL_MQTT_URI,
    mqtt_cfg.broker.address.port = SNAIL_MQTT_PORT,
    mqtt_cfg.credentials.username = SNAIL_MQTT_USERNAME;
    mqtt_cfg.credentials.client_id = g_client_id;
    mqtt_cfg.credentials.authentication.password = SNAIL_MQTT_PASSWD;
    mqtt_cfg.session.keepalive = 120;
    mqtt_cfg.session.last_will.topic = g_last_will_top;
    mqtt_cfg.session.last_will.msg = SNAIL_MQTT_PAYLOAD_OFFLINE;
    mqtt_cfg.buffer.size = 512;
    mqtt_cfg.buffer.out_size = 512;

#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0)
    {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128)
        {
            int c = fgetc(stdin);
            if (c == '\n')
            {
                line[count] = '\0';
                break;
            }
            else if (c > 0 && c < 127)
            {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.broker.address.uri = line;
        printf("Broker url: %s\n", line);
    }
    else
    {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    g_client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(g_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(g_client);
}

void mqttPushTopic(const char *topic, const char *payload)
{
    if (false == g_mqtt_connected)
        return;

    char new_topic[SNAIL_MQTT_NORMAL_TOPIC_MAX_SIZE] = {0};
    snprintf(new_topic, SNAIL_MQTT_NORMAL_TOPIC_MAX_SIZE, "%s%s", g_top_head, topic);
    // 主动发布消息
    int msg_id = esp_mqtt_client_publish(g_client, new_topic, payload, strlen(payload), 1, 0);
}