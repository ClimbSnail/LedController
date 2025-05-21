/* OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "string.h"
#ifdef CONFIG_EXAMPLE_USE_CERT_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include <sys/socket.h>
#include "esp_wifi.h"
#include "common.h"
#include "config.h"
#include "netota.h"
#include "networker_data.h"

#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_tls.h"
#include "esp_http_client.h"
#include "ArduinoJson.h"
#define MAX_HTTP_OUTPUT_BUFFER 2048

#define HASH_LEN 32

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF
/* The interface name value can refer to if_desc in esp_netif_defaults.h */
#if CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF_ETH
static const char *bind_interface_name = EXAMPLE_NETIF_DESC_ETH;
#elif CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF_STA
static const char *bind_interface_name = EXAMPLE_NETIF_DESC_STA;
#endif
#endif

static const char *TAG = "simple_ota_example";
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

static OtaData *g_otaData; // 固件升级数据

#define OTA_URL_SIZE 256

esp_err_t _http_ota_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
    {
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        // Clean the buffer in case of a new request
        if (output_len == 0 && evt->user_data)
        {
            // we are just starting to copy the output data into the use
            memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
        }
        /*
         *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
         *  However, event handler can also be used in case chunked encoding is used.
         */
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // If user_data buffer is configured, copy the response into the buffer
            int copy_len = 0;
            if (evt->user_data)
            {
                // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                if (copy_len)
                {
                    memcpy(evt->user_data + output_len, evt->data, copy_len);
                }
            }
            else
            {
                int content_len = esp_http_client_get_content_length(evt->client);
                if (output_buffer == NULL)
                {
                    // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                    output_buffer = (char *)calloc(content_len + 1, sizeof(char));
                    output_len = 0;
                    if (output_buffer == NULL)
                    {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                copy_len = MIN(evt->data_len, (content_len - output_len));
                if (copy_len)
                {
                    memcpy(output_buffer + output_len, evt->data, copy_len);
                }
            }
            output_len += copy_len;
        }
    }
    break;
    case HTTP_EVENT_ON_FINISH:
    {
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
    }
    break;
    case HTTP_EVENT_DISCONNECTED:
    {
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        if (output_buffer != NULL)
        {
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
    }
    break;
    case HTTP_EVENT_REDIRECT:
    {
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        esp_http_client_set_header(evt->client, "From", "user@example.com");
        esp_http_client_set_header(evt->client, "Accept", "text/html");
        esp_http_client_set_redirection(evt->client);
    }
    break;
    }
    return ESP_OK;
}

static char payloadJson[256] = OTA_URL_PATH;
static DynamicJsonDocument sentData(256);
static DynamicJsonDocument retData(384);
bool get_updata_info(OtaData *otaData)
{
    bool ret = false;
    // Declare local_response_buffer with size (MAX_HTTP_OUTPUT_BUFFER + 1) to prevent out of bound access when
    // it is used by functions like strlen(). The buffer should only be used upto size MAX_HTTP_OUTPUT_BUFFER
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

    memset(payloadJson + strlen(OTA_URL_PATH), 0, sizeof(payloadJson) - strlen(OTA_URL_PATH));

    // static DynamicJsonDocument sentData(256);
    sentData["userId"] = otaData->userId;
    sentData["action"] = "update";
    sentData["data"]["hwVer"] = CTRL_HW_VERSION;
    sentData["data"]["nowVer"] = otaData->nowVer;
    size_t length = serializeJson(sentData, payloadJson + strlen(OTA_URL_PATH), sizeof(payloadJson));

    // SH_LOG("OTA payloadJsonData = %s\n", payloadJson);
    /**
     * NOTE: All the configuration parameters for http_client must be spefied either in URL or as host and path parameters.
     * If host and path parameters are not set, query parameter will be ignored. In such cases,
     * query parameter should be specified in URL.
     *
     * If URL as well as host and path parameters are specified, values of host and path will be considered.
     */
    esp_http_client_config_t config = {
        // .host = OTA_URL_HOST,
        // .path = payloadJson,
        // .query = "esp",
        // .event_handler = _http_event_handler,
        // .user_data = local_response_buffer, // Pass address of local buffer to get response
        // .disable_auto_redirect = true,
    };
    // config.url = OTA_URL_ALL;
    config.host = OTA_URL_HOST;
    config.port = OTA_URL_PORT;
    config.path = payloadJson;
    config.query = DEVICE_NAME;
    config.event_handler = _http_event_handler;
    config.user_data = local_response_buffer; // Pass address of local buffer to get response
    config.disable_auto_redirect = true;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    // GET
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        SH_LOG("OTA local_response_buffer = %s\n", local_response_buffer);

        deserializeJson(retData, local_response_buffer);
        if (retData.containsKey("retCode") && retData["retCode"].as<int>() == 0)
        {
            otaData->updataLevel = (UPDATA_LEVEL)retData["data"]["updataLevel"].as<int>();
            snprintf(otaData->newVer, sizeof(otaData->newVer),
                     "%s", retData["data"]["newVer"].as<string>().c_str());
            snprintf(otaData->firmwareUrl, sizeof(otaData->firmwareUrl),
                     "%s", retData["data"]["firmwareUrl"].as<string>().c_str());
            snprintf(otaData->firmwareMD5, sizeof(otaData->firmwareMD5),
                     "%s", retData["data"]["firmwareMD5"].as<string>().c_str());
        }
        ret = true;
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, strlen(local_response_buffer));

    // // POST
    // const char *post_data = "{\"field1\":\"value1\"}";
    // esp_http_client_set_url(client, "http://" CONFIG_EXAMPLE_HTTP_ENDPOINT "/post");
    // esp_http_client_set_method(client, HTTP_METHOD_POST);
    // esp_http_client_set_header(client, "Content-Type", "application/json");
    // esp_http_client_set_post_field(client, post_data, strlen(post_data));
    // err = esp_http_client_perform(client);
    // if (err == ESP_OK)
    // {
    //     ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %" PRId64,
    //              esp_http_client_get_status_code(client),
    //              esp_http_client_get_content_length(client));
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    // }

    esp_http_client_cleanup(client);
    return ret;
}

void simple_ota_example_task(void *pvParameter)
{
    SH_LOG("Starting SH_OTA task");
    g_otaData->updateResult = UPDATE_PROGRESS_UPGRADING;
#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF
    esp_netif_t *netif = get_example_netif_from_desc(bind_interface_name);
    if (netif == NULL)
    {
        SH_LOG("Can't find netif from interface description");
        abort();
    }
    struct ifreq ifr;
    esp_netif_get_netif_impl_name(netif, ifr.ifr_name);
    SH_LOG("Bind interface name is %s", ifr.ifr_name);
#endif
    esp_http_client_config_t config = {};
    config.url = g_otaData->firmwareUrl;
#ifdef CONFIG_EXAMPLE_USE_CERT_BUNDLE
    config.crt_bundle_attach = esp_crt_bundle_attach;
#else
    config.cert_pem = (char *)server_cert_pem_start;
#endif /* CONFIG_EXAMPLE_USE_CERT_BUNDLE */
    config.event_handler = _http_ota_event_handler;
    config.keep_alive_enable = true;
#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_BIND_IF
    config.if_name = &ifr;
#endif

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
    char url_buf[OTA_URL_SIZE];
    if (strcmp(config.url, "FROM_STDIN") == 0)
    {
        example_configure_stdin_stdout();
        fgets(url_buf, OTA_URL_SIZE, stdin);
        int len = strlen(url_buf);
        url_buf[len - 1] = '\0';
        config.url = url_buf;
    }
    else
    {
        SH_LOG("Configuration mismatch: wrong firmware upgrade image url");
        abort();
    }
#endif

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_https_ota_config_t ota_config = {
        .http_config = &config};
    // ota_config.http_config = &config

    SH_LOG("Attempting to download update from %s", config.url);
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK)
    {
        SH_LOG("OTA Succeed, Rebooting...");
        g_otaData->updateResult = UPDATE_PROGRESS_SUCC;
        // esp_restart();
    }
    else
    {
        SH_LOG("Firmware upgrade failed");
        g_otaData->updateResult = UPDATE_PROGRESS_FAIL;
    }
    vTaskDelete(NULL); // 会删除整个调用栈
}

static void print_sha256(const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i)
    {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    SH_LOG("%s %s", label, hash_print);
}

static void get_sha256_of_partitions(void)
{
    uint8_t sha_256[HASH_LEN] = {0};
    esp_partition_t partition;

    // get sha256 digest for bootloader
    partition.address = ESP_BOOTLOADER_OFFSET;
    partition.size = ESP_PARTITION_TABLE_OFFSET;
    partition.type = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");
}

void ota_start(OtaData *otaData)
{
    g_otaData = otaData;
    get_sha256_of_partitions();

    /* Ensure to disable any WiFi power save mode, this allows best throughput
     * and hence timings for overall OTA operation.
     */
    esp_wifi_set_ps(WIFI_PS_NONE);

    xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
}
