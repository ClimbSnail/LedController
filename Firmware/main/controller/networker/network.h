#ifndef NETWORK_H
#define NETWORK_H

#include "config.h"

// 时区偏移(小时) 8*60*60
#define TIMEZERO_OFFSIZE (28800000)

#define CONN_SUCC 0
#define CONN_ERROR 1
#define CONN_ERR_TIMEOUT 15 // 连接WiFi的超时时间（s）

// wifi是否连接标志
#define AP_DISABLE 0
#define AP_ENABLE 1

#define SH_AP_SSID DEVICE_NAME
#define SH_PASSWD_SSID ""
#define WIFI_RSSI_MIN_DISCONNECT -127

#include "esp_wifi_types.h"

bool sh_wifi_init_sta(wifi_mode_t mode,
                      const char *ssid_0, const char *password_0,
                      const char *ssid_1, const char *password_1,
                      const char *ssid_2, const char *password_2);
bool sh_wifi_init_softap(void);
char *get_ip_str(void);
int8_t sh_get_connect_rssi(void);

#endif
