#ifndef NETOTA_H
#define NETOTA_H

#include "networker_data.h"

#define OTA_URL_ALL "http://climbsnail.cn:5001/ledc/sn/v1/LEDC1"
#define OTA_URL_HOST "climbsnail.cn"
#define OTA_URL_PORT 5001
#define OTA_URL_PATH "/ledc/sn/v1/LEDC1/"

bool get_updata_info(OtaData *otaData);
void ota_start(OtaData *otaData);

#endif // NETOTA_H
