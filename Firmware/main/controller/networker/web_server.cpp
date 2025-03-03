
#include "config.h"

#ifdef NETWORKABLE
#include "web_server.h"
#include "networker.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_http_server.h"
#include "ArduinoJson.h"
#include "common.h"

#define BUFFER_LEN 1024

typedef struct
{
    char data[BUFFER_LEN];
    int len;
    int client_socket;
} DATA_PARCEL;

static NetworkerConfig *s_networkerCfg = NULL;
static LedUtilConfig *s_ledUtilCfg = NULL;

static httpd_handle_t web_server_handle = NULL;   // ws服务器唯一句柄
static QueueHandle_t ws_server_rece_queue = NULL; // 收到的消息传给任务处理
static QueueHandle_t ws_server_send_queue = NULL; // 异步发送队列

/*此处只是管理ws socket server发送时的对象，以确保多客户端连接的时候都能收到数据，并不能限制HTTP请求*/
#define WS_CLIENT_QUANTITY_ASTRICT 5                   // 客户端数量
static int WS_CLIENT_LIST[WS_CLIENT_QUANTITY_ASTRICT]; // 客户端套接字列表
static int WS_CLIENT_NUM = 0;                          // 实际连接数量

/*客户端列表 记录客户端套接字*/
static void ws_client_list_add(int socket)
{
    /*检查是否超出限制*/
    if (WS_CLIENT_NUM >= WS_CLIENT_QUANTITY_ASTRICT)
    {
        return;
    }

    /*检查是否重复*/
    for (size_t i = 0; i < WS_CLIENT_QUANTITY_ASTRICT; i++)
    {
        if (WS_CLIENT_LIST[i] == socket)
        {
            return;
        }
    }

    /*添加套接字至列表中*/
    for (size_t i = 0; i < WS_CLIENT_QUANTITY_ASTRICT; i++)
    {
        if (WS_CLIENT_LIST[i] <= 0)
        {
            WS_CLIENT_LIST[i] = socket; // 获取返回信息的客户端套接字
            printf("ws_client_list_add:%d\r\n", socket);
            WS_CLIENT_NUM++;
            return;
        }
    }
}

/*客户端列表 删除客户端套接字*/
static void ws_client_list_delete(int socket)
{
    for (size_t i = 0; i < WS_CLIENT_QUANTITY_ASTRICT; i++)
    {
        if (WS_CLIENT_LIST[i] == socket)
        {
            WS_CLIENT_LIST[i] = 0;
            printf("ws_client_list_delete:%d\r\n", socket);
            WS_CLIENT_NUM--;
            if (WS_CLIENT_NUM < 0)
            {
                WS_CLIENT_NUM = 0;
            }
            break;
        }
    }
}

static DATA_PARCEL ws_rece_parcel;
/*ws服务器接收数据*/
static esp_err_t ws_server_rece_data(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        ws_client_list_add(httpd_req_to_sockfd(req));
        return ESP_OK;
    }
    esp_err_t ret = ESP_FAIL;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    memset(&ws_rece_parcel, 0, sizeof(DATA_PARCEL));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = (uint8_t *)ws_rece_parcel.data; // 指向缓存区
    ret = httpd_ws_recv_frame(req, &ws_pkt, 0);      // 设置参数max_len = 0来获取帧长度
    if (ret != ESP_OK)
    {
        printf("ws_server_rece_data data receiving failure!");
        return ret;
    }
    if (ws_pkt.len > 0)
    {
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len); /*设置参数max_len 为 ws_pkt.len以获取帧有效负载 */
        if (ret != ESP_OK)
        {
            printf("ws_server_rece_data data receiving failure!");
            return ret;
        }
        ws_rece_parcel.len = ws_pkt.len;
        ws_rece_parcel.client_socket = httpd_req_to_sockfd(req);
        if (xQueueSend(ws_server_rece_queue, &ws_rece_parcel, pdMS_TO_TICKS(1)))
        {
            ret = ESP_OK;
        }
    }
    else
    {
        printf("ws_pkt.len<=0");
    }
    return ret;
}

static DATA_PARCEL async_buffer = {0};
/*异步发送函数，将其放入HTTPD工作队列*/
static void ws_async_send(void *arg)
{
    if (xQueueReceive(ws_server_send_queue, &async_buffer, 0))
    {
        httpd_ws_frame_t ws_pkt = {0};
        ws_pkt.payload = (uint8_t *)async_buffer.data;
        ws_pkt.len = async_buffer.len;
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        httpd_ws_send_frame_async(web_server_handle, async_buffer.client_socket, &ws_pkt);
    }
}

/*ws 发送函数*/
static DATA_PARCEL send_buffer;
static void ws_server_send(const char *data, uint32_t len, int client_socket)
{
    memset(&send_buffer, 0, sizeof(send_buffer));
    send_buffer.client_socket = client_socket;
    send_buffer.len = len;
    memcpy(send_buffer.data, data, len);
    xQueueSend(ws_server_send_queue, &send_buffer, pdMS_TO_TICKS(1));
    httpd_queue_work(web_server_handle, ws_async_send, NULL); // 进入排队
}

/*WEB SOCKET*/
static const httpd_uri_t ws_uri = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = ws_server_rece_data,
    .user_ctx = NULL,
    .is_websocket = true};

/*首页HTML GET处理程序 */
static esp_err_t home_get_handler(httpd_req_t *req)
{
    extern const unsigned char upload_script_start[] asm("_binary_web_page_html_start"); /*web_page.html文件在bin中的位置*/
    extern const unsigned char upload_script_end[] asm("_binary_web_page_html_end");
    const size_t upload_script_size = (upload_script_end - upload_script_start);
    httpd_resp_send(req, (const char *)upload_script_start, upload_script_size);

    return ESP_OK;
}

/*首页HTML*/
static const httpd_uri_t home_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = home_get_handler,
    .user_ctx = NULL};

/*http事件处理*/
static void ws_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == ESP_HTTP_SERVER_EVENT)
    {
        switch (event_id)
        {
        case HTTP_SERVER_EVENT_ERROR: // 当执行过程中出现错误时，将发生此事件
            break;
        case HTTP_SERVER_EVENT_START: // 此事件在HTTP服务器启动时发生
            break;
        case HTTP_SERVER_EVENT_ON_CONNECTED: // 一旦HTTP服务器连接到客户端，就不会执行任何数据交换
            break;
        case HTTP_SERVER_EVENT_ON_HEADER: // 在接收从客户端发送的每个报头时发生
            break;
        case HTTP_SERVER_EVENT_HEADERS_SENT: // 在将所有标头发送到客户端之后
            break;
        case HTTP_SERVER_EVENT_ON_DATA: // 从客户端接收数据时发生
            break;
        case HTTP_SERVER_EVENT_SENT_DATA: // 当ESP HTTP服务器会话结束时发生
            break;
        case HTTP_SERVER_EVENT_DISCONNECTED: // 连接已断开
        {
            esp_http_server_event_data *event = (esp_http_server_event_data *)event_data;
            ws_client_list_delete(event->fd);
        }
        break;
        case HTTP_SERVER_EVENT_STOP: // 当HTTP服务器停止时发生此事件
            break;
        default:
            break;
        }
    }
}

/*数据发送任务，每隔一秒发送一次*/
static void ws_server_send_task(void *p)
{
    uint32_t task_count = 0;
    char payloadJson[256];
    while (1)
    {
        memset(payloadJson, 0, sizeof(payloadJson));
        sprintf(payloadJson, DEVICE_NAME);

        for (size_t i = 0; i < WS_CLIENT_QUANTITY_ASTRICT; i++)
        {
            if (WS_CLIENT_LIST[i] > 0)
            {
                ws_server_send(payloadJson, strlen(payloadJson), WS_CLIENT_LIST[i]);
            }
        }

        task_count++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*数据接收处理任务*/
static DATA_PARCEL rece_buffer;
static char sentPayloadJson[320];
static void ws_server_rece_task(void *p)
{
    while (1)
    {
        // vTaskDelay(pdMS_TO_TICKS(50));
        if (xQueueReceive(ws_server_rece_queue, &rece_buffer, portMAX_DELAY))
        {
            DynamicJsonDocument data(320);
            deserializeJson(data, rece_buffer.data);
            // printf("\nSocket : %d\tdata len : %d\tpayload : %s\r\n",
            //        rece_buffer.client_socket, rece_buffer.len, rece_buffer.data);
            if (data.containsKey("action"))
            {
                if (!data["action"].as<string>().compare("saveConfig"))
                {
                    s_networkerCfg->ssid_1 = data["ssid_1"].as<string>();
                    snprintf(s_networkerCfg->password_1, sizeof(s_networkerCfg->password_1), "%s",
                             data["password_1"].as<string>().c_str());
                    s_networkerCfg->ssid_2 = data["ssid_2"].as<string>();
                    snprintf(s_networkerCfg->password_2, sizeof(s_networkerCfg->password_2), "%s",
                             data["password_2"].as<string>().c_str());
                    snprintf(s_networkerCfg->snailID, sizeof(s_networkerCfg->snailID), "%s",
                             data["snailID"].as<string>().c_str());

                    // snprintf(s_networkerCfg->cityCode, sizeof(s_networkerCfg->cityCode),
                    //          "%s", data["cityCode"].as<string>().c_str());
                    // snprintf(s_networkerCfg->apiKey, sizeof(s_networkerCfg->apiKey),
                    //          "%s", data["apiKey"].as<string>().c_str());
                    s_ledUtilCfg->ledPanelType = (LED_PANEL_TYPE)data["ledPanelType"].as<unsigned char>();
                    s_ledUtilCfg->coolTemp = constrain(data["coolTemp"].as<unsigned char>(), 10, 80);
                    s_ledUtilCfg->colorRgb = data["colorRgb"].as<unsigned int>();
                    s_ledUtilCfg->colorTemperature = constrain(data["colorTemperature"].as<unsigned int>(),
                                                               LED_CT_LOW, LED_CT_HIGH);
                    s_networkerCfg->timeUpdataInterval = data["timeUpdataInterval"].as<unsigned long>();
                    g_webSaveConfigFlag = true;
                }
                else if (!data["action"].as<string>().compare("readConfig"))
                {
                    memset(sentPayloadJson, 0, sizeof(sentPayloadJson));
                    char ledPanelType[4];
                    char coolTemp[4];
                    char timeUpdataInterval[16];
                    char colorRgb[16];
                    char colorTemperature[8];
                    char colorTempLow[8];
                    char colorTempHigh[8];
                    snprintf(ledPanelType, sizeof(ledPanelType), "%u", s_ledUtilCfg->ledPanelType);
                    snprintf(coolTemp, sizeof(coolTemp), "%u", s_ledUtilCfg->coolTemp);
                    snprintf(colorRgb, sizeof(colorRgb), "%lu", s_ledUtilCfg->colorRgb);
                    snprintf(colorTemperature, sizeof(colorTemperature), "%u", s_ledUtilCfg->colorTemperature);
                    snprintf(colorTempLow, sizeof(colorTempLow), "%u", LED_CT_LOW);
                    snprintf(colorTempHigh, sizeof(colorTempHigh), "%u", LED_CT_HIGH);
                    snprintf(timeUpdataInterval, sizeof(timeUpdataInterval),
                             "%lu", s_networkerCfg->timeUpdataInterval);

                    data["ssid_1"] = s_networkerCfg->ssid_1;
                    data["password_1"] = s_networkerCfg->password_1;
                    data["ssid_2"] = s_networkerCfg->ssid_2;
                    data["password_2"] = s_networkerCfg->password_2;
                    data["snailID"] = s_networkerCfg->snailID;
                    // data["cityCode"] = s_networkerCfg->cityCode;
                    // data["apiKey"] = s_networkerCfg->apiKey;
                    data["ledPanelType"] = ledPanelType;
                    data["coolTemp"] = coolTemp;
                    data["timeUpdataInterval"] = timeUpdataInterval;
                    data["colorRgb"] = colorRgb;
                    data["colorTemperature"] = colorTemperature;
                    data["colorTempLow"] = colorTempLow;
                    data["colorTempHigh"] = colorTempHigh;
                    size_t length = serializeJson(data, sentPayloadJson, sizeof(sentPayloadJson));
                    // printf("Sent %s\r\n", sentPayloadJson);

                    for (size_t i = 0; i < WS_CLIENT_QUANTITY_ASTRICT; i++)
                    {
                        if (WS_CLIENT_LIST[i] > 0)
                        {
                            ws_server_send(sentPayloadJson, length, WS_CLIENT_LIST[i]);
                        }
                    }
                }
            }
        }
    }
}

/*web服务器初始化*/
void web_server_init(NetworkerConfig *networkerCfg, LedUtilConfig *ledUtilCfg)
{
    s_networkerCfg = networkerCfg;
    s_ledUtilCfg = ledUtilCfg;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // config.uri_match_fn = httpd_uri_match_wildcard; // 使用URI通配符匹配函数
    // config.stack_size = 5 * 1024;
    config.core_id = TASK_HTTPD_CORE_ID; // 分配任务核心
    // 启动httpd服务器
    if (httpd_start(&web_server_handle, &config) == ESP_OK)
    {
        printf("web_server_start\r\n");
    }
    else
    {
        printf("Error starting ws server!");
    }

    esp_event_handler_instance_register(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID,
                                        &ws_event_handler, NULL, NULL); // 任何事件注册处理程序
    httpd_register_uri_handler(web_server_handle, &home_uri);           // 注册uri处理程序
    httpd_register_uri_handler(web_server_handle, &ws_uri);             // 注册uri处理程序

    /*创建接收队列*/
    ws_server_rece_queue = xQueueCreate(3, sizeof(DATA_PARCEL));
    if (ws_server_rece_queue == NULL)
    {
        printf("ws_server_rece_queue ERROR\r\n");
    }

    /*创建发送队列*/
    ws_server_send_queue = xQueueCreate(3, sizeof(DATA_PARCEL));
    if (ws_server_send_queue == NULL)
    {
        printf("ws_server_send_queue ERROR\r\n");
    }

    BaseType_t xReturn;
    /*创建接收处理任务*/
    xReturn = xTaskCreatePinnedToCore(ws_server_rece_task,
                                      "ws_server_rece_task",
                                      5 * 1024,
                                      NULL, TASK_WRB_SERVER_RIORITY, NULL,
                                      TASK_WRB_SERVER_CORE_ID);
    if (xReturn != pdPASS)
    {
        printf("xTaskCreatePinnedToCore ws_server_rece_task error!\r\n");
    }

    // /*创建发送任务*/
    // xReturn = xTaskCreatePinnedToCore(ws_server_send_task,
    //                                   "ws_server_send_task", 4096,
    //                                   NULL, 15, NULL,
    //                                   tskNO_AFFINITY);
    // if (xReturn != pdPASS)
    // {
    //     printf("xTaskCreatePinnedToCore ws_server_send_task error!\r\n");
    // }
}

#endif // NETWORKABLE