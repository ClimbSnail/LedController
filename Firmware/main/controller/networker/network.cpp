/* Scan Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
    This example shows how to scan for available set of APs.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi_types.h"
#include "nvs_flash.h"
#include "network.h"
#if IP_NAPT
#include "lwip/lwip_napt.h"
#endif
#include "common.h"
#include "sh_hal/hal_esp.h"

#include "freertos/task.h"
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sys.h"

// #define DEFAULT_SCAN_LIST_SIZE CONFIG_EXAMPLE_SCAN_LIST_SIZE
#define DEFAULT_SCAN_LIST_SIZE CONFIG_WIFI_PROV_SCAN_MAX_ENTRIES

static const char *TAG = "scan";
static char ip_str[16] = "192.168.4.1";
static esp_netif_t *esp_netif_ap = NULL;
static esp_netif_t *esp_netif_sta = NULL;
static int8_t s_connectRssi = WIFI_RSSI_MIN_DISCONNECT; // 已连接的wifi网络信号强度
wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];

char *get_ip_str(void)
{
    return ip_str;
}

bool sh_wifi_config_sta_ssid(const char *ssid, const char *password);

bool sh_wifi_init_softap(void);

static void print_auth_mode(int authmode)
{
    switch (authmode)
    {
    case WIFI_AUTH_OPEN:
        SH_LOG("Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_OWE:
        SH_LOG("Authmode \tWIFI_AUTH_OWE");
        break;
    case WIFI_AUTH_WEP:
        SH_LOG("Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        SH_LOG("Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        SH_LOG("Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        SH_LOG("Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_ENTERPRISE:
        SH_LOG("Authmode \tWIFI_AUTH_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        SH_LOG("Authmode \tWIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        SH_LOG("Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA3_ENT_192:
        SH_LOG("Authmode \tWIFI_AUTH_WPA3_ENT_192");
        break;
    case WIFI_AUTH_WPA3_EXT_PSK:
        SH_LOG("Authmode \tWIFI_AUTH_WPA3_EXT_PSK");
        break;
    case WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE:
        SH_LOG("Authmode \tWIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE");
        break;
    default:
        SH_LOG("Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}

static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher)
    {
    case WIFI_CIPHER_TYPE_NONE:
        SH_LOG("Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        SH_LOG("Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        SH_LOG("Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        SH_LOG("Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        SH_LOG("Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        SH_LOG("Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_AES_CMAC128:
        SH_LOG("Pairwise Cipher \tWIFI_CIPHER_TYPE_AES_CMAC128");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        SH_LOG("Pairwise Cipher \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        SH_LOG("Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        SH_LOG("Pairwise Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    default:
        SH_LOG("Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher)
    {
    case WIFI_CIPHER_TYPE_NONE:
        SH_LOG("Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        SH_LOG("Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        SH_LOG("Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        SH_LOG("Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        SH_LOG("Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        SH_LOG("Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    case WIFI_CIPHER_TYPE_SMS4:
        SH_LOG("Group Cipher \tWIFI_CIPHER_TYPE_SMS4");
        break;
    case WIFI_CIPHER_TYPE_GCMP:
        SH_LOG("Group Cipher \tWIFI_CIPHER_TYPE_GCMP");
        break;
    case WIFI_CIPHER_TYPE_GCMP256:
        SH_LOG("Group Cipher \tWIFI_CIPHER_TYPE_GCMP256");
        break;
    default:
        SH_LOG("Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}

static int s_retry_num = 0;
static uint16_t ap_timeout = 0; // ap无连接的超时时间

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group = NULL;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define EXAMPLE_ESP_MAXIMUM_RETRY 3
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER "" // CONFIG_ESP_WIFI_PW_ID

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        SH_LOG("Station " MACSTR " joined, AID=%d",
               MAC2STR(event->mac), event->aid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        SH_LOG("Station " MACSTR " left, AID=%d",
               MAC2STR(event->mac), event->aid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        // SH_LOG("Station started");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        SH_LOG("Got IP: %s", ip_str);
        s_retry_num = 0;

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        // 获取信号强度
        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        s_connectRssi = ap_info.rssi;
        SH_LOG("Signal strength: %d dBm\n", s_connectRssi);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            s_connectRssi = WIFI_RSSI_MIN_RECONNECT; // 已断开的wifi网络信号强度（正在重连）
            esp_wifi_connect();
            s_retry_num++;
            // SH_LOG("retry to connect to the AP");
        }
        else
        {
            s_connectRssi = WIFI_RSSI_MIN_DISCONNECT; // 已断开的wifi网络信号强度
            s_retry_num = EXAMPLE_ESP_MAXIMUM_RETRY;
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    }
}

int8_t sh_get_connect_rssi(void)
{
    // WIFI_RSSI_MIN_DISCONNECT 为未连接
    return s_connectRssi;
}

void sh_wifi_init(void)
{
    static bool isInitWifi = false;
    if (false == isInitWifi)
    {
        isInitWifi = true;
        // 如下初始化只执行一次

        // Initialize NVS
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());

        esp_netif_ap = esp_netif_create_default_wifi_ap();
        esp_netif_sta = esp_netif_create_default_wifi_sta();

        char hostname[32] = {0};
        snprintf(hostname, 32, "%s_%02X", DEVICE_NAME,
                 (uint16_t)SH_HAL::getChipID());
        esp_netif_set_hostname(esp_netif_sta, hostname);

        /*Initialize WiFi */
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    }
}

/* Initialize soft AP */
bool sh_wifi_init_softap(void)
{
    sh_wifi_init();

    wifi_config_t wifi_config = {
        .ap = {},
    };
    snprintf((char *)wifi_config.ap.ssid, 32, "%s_%02X", SH_AP_SSID,
             (uint16_t)SH_HAL::getChipID());
    wifi_config.ap.ssid_len = strlen((char *)wifi_config.ap.ssid);
    wifi_config.ap.channel = 1;
    strcpy((char *)wifi_config.ap.password, SH_PASSWD_SSID);
    wifi_config.ap.max_connection = 3;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK; // WIFI_AUTH_WPA2_PSK;
    wifi_config.ap.pmf_cfg.required = false;

    if (0 == strlen(SH_PASSWD_SSID))
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    // WIFI_MODE_APSTA WIFI_MODE_STA WIFI_MODE_AP
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    // WIFI_IF_AP WIFI_IF_AP_STA
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    SH_LOG("sh_wifi_init_softap finished.\nSSID:%s password:%s channel:%d\n",
           (char *)wifi_config.ap.ssid, (char *)wifi_config.ap.password,
           wifi_config.ap.channel);

    return true;
}

bool sh_wifi_init_sta(wifi_mode_t mode,
                      const char *ssid_0, const char *password_0,
                      const char *ssid_1, const char *password_1,
                      const char *ssid_2, const char *password_2)
{

    sh_wifi_init();

    const char *ssid[3] = {ssid_0, ssid_1, ssid_2};
    const char *password[3] = {password_0, password_1, password_2};
    // 等待连接的
    const char *wait_ssid[3];
    const char *wait_password[3];
    int8_t wait_rssi[3];
    int wait_num = 0;                         // 可连接的wifi数量统计
    bool connected = false;                   // 是否连接成功
    s_connectRssi = WIFI_RSSI_MIN_DISCONNECT; // 已连接的wifi网络信号强度

    // 开始扫描WIFI
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    // WIFI_MODE_APSTA WIFI_MODE_STA WIFI_MODE_AP
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    // Set a new hostname
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_scan_start(NULL, true);
    SH_LOG("Max AP number ap_info can hold = %u", number);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    // SH_LOG("Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);
    // for (int i = 0; i < number; i++)
    // {
    //     SH_LOG("SSID \t\t%s", ap_info[i].ssid);
    //     SH_LOG("RSSI \t\t%d", ap_info[i].rssi);
    //     // print_auth_mode(ap_info[i].authmode);
    //     // if (ap_info[i].authmode != WIFI_AUTH_WEP)
    //     // {
    //     //     print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
    //     // }
    //     // SH_LOG("Channel \t\t%d", ap_info[i].primary);
    // }
    for (int ind = 0; ind < 3; ++ind)
    {
        for (int i = 0; i < number; i++)
        {
            if (strcmp((char *)ap_info[i].ssid, ssid[ind]) == 0)
            {
                wait_ssid[wait_num] = ssid[ind];
                wait_password[wait_num] = password[ind];
                wait_rssi[wait_num] = ap_info[i].rssi;
                ++wait_num;
                // 列表里有可以连接的AP
                SH_LOG("Connect to ap SSID:%s password:%s",
                       ssid[ind], password[ind]);
            }
        }
    }

    /* Register Event handler */
    static esp_event_handler_instance_t instance_wifi_event = NULL;
    static esp_event_handler_instance_t instance_ip_event = NULL;

    if(NULL == instance_wifi_event)
    {
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &instance_wifi_event));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &instance_ip_event));
    }

    if (NULL == s_wifi_event_group)
    {
        /* Initialize event group */
        s_wifi_event_group = xEventGroupCreate();
        // vEventGroupDelete(s_wifi_event_group);
    }

    // 如果都没有搜到，就挨个搜索（因为有可能周围的wifi信号太多了，目标未在列表内）
    if (0 == wait_num)
    {
        for (int pos = 0; pos < 3; ++pos)
        {
            wait_ssid[wait_num] = ssid[pos];
            wait_password[wait_num] = password[pos];
            ++wait_num;
        }
    }

    for (int ind = 0; ind < wait_num; ++ind)
    {
        s_retry_num = 0;
        ESP_ERROR_CHECK(esp_wifi_stop());

        sh_wifi_config_sta_ssid(wait_ssid[ind], wait_password[ind]);

        /* Start WiFi */
        ESP_ERROR_CHECK(esp_wifi_start());

        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

        /*
         * Wait until either the connection is established (WIFI_CONNECTED_BIT) or
         * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
         * The bits are set by event_handler() (see above)
         */
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               portMAX_DELAY);

        /* xEventGroupWaitBits() returns the bits before the call returned,
         * hence we can test which event actually happened. */
        if (bits & WIFI_CONNECTED_BIT)
        {
            SH_LOG("Connected to ap SSID:%s password:%s",
                   wait_ssid[ind], wait_password[ind]);
            connected = true;

            // 获取信号强度
            // s_connectRssi = wait_rssi[ind];
            break;
        }
        else if (bits & WIFI_FAIL_BIT)
        {
            SH_LOGE("Failed to connect to SSID:%s, password:%s",
                    wait_ssid[ind], wait_password[ind]);
        }
        else
        {
            SH_LOG("UNEXPECTED EVENT");
            // return false;
        }
    }

    // ESP_ERROR_CHECK(esp_wifi_start());

    // /* Set sta as the default interface */
    // // esp_netif_sta esp_netif_ap
    // esp_netif_set_default_netif(esp_netif_ap);
    // /* Enable napt on the AP netif */
    // if (esp_netif_napt_enable(esp_netif_ap) != ESP_OK)
    // {
    //     SH_LOG("NAPT not enabled on the netif: %p", esp_netif_ap);
    // }

    if (false == connected)
    {
        /* UnRegister Event handler */
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT,
                                                              ESP_EVENT_ANY_ID,
                                                              instance_wifi_event));
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,
                                                              IP_EVENT_STA_GOT_IP,
                                                              instance_ip_event));
        instance_wifi_event = NULL;
        instance_ip_event = NULL;
        return false;
    }

    SH_LOG("Network init finished.");

    return true;
}

bool sh_wifi_config_sta_ssid(const char *ssid, const char *password)
{
    wifi_config_t wifi_sta_config = {
        .sta = {},
    };
    strcpy((char *)wifi_sta_config.sta.ssid, ssid);
    strcpy((char *)wifi_sta_config.sta.password, password);
    wifi_sta_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    wifi_sta_config.sta.failure_retry_cnt = EXAMPLE_ESP_MAXIMUM_RETRY;
    wifi_sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_sta_config.sta.sae_pwe_h2e = ESP_WIFI_SAE_MODE;
    wifi_sta_config.sta.pmf_cfg.capable = true;
    wifi_sta_config.sta.pmf_cfg.required = false;
    strcpy((char *)wifi_sta_config.sta.sae_h2e_identifier, EXAMPLE_H2E_IDENTIFIER);

    // WIFI_MODE_APSTA WIFI_MODE_STA WIFI_MODE_AP
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // WIFI_IF_AP WIFI_IF_AP_STA
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));

    return true;
}

/* Initialize Wi-Fi as sta and set scan method */
void wifi_scan(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    // wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_scan_start(NULL, true);
    SH_LOG("Max AP number ap_info can hold = %u", number);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    SH_LOG("Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);
    for (int i = 0; i < number; i++)
    {
        SH_LOG("SSID \t\t%s", ap_info[i].ssid);
        SH_LOG("RSSI \t\t%d", ap_info[i].rssi);
        print_auth_mode(ap_info[i].authmode);
        if (ap_info[i].authmode != WIFI_AUTH_WEP)
        {
            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }
        SH_LOG("Channel \t\t%d", ap_info[i].primary);
    }
}