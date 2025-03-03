
#include "sh_hal/hal.h"
#include "config.h"
#include <inttypes.h>
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"

#include "soc/soc_caps.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_continuous.h"

#include "esp_log.h"

#include "driver/touch_pad.h"
#include "common.h"

#define ESP_INTR_FLAG_DEFAULT 0

struct HAL_ADC_INFO
{
    adc_channel_t channel;                 // 通道
    adc_unit_t unit_id;                    // 转化单元
    uint8_t bitwidth;                      // 位宽
    bool do_calibration;                   // 是否可校准
    SH_HAL::SH_ADC_ATTEN attenuation;      // 增益
    adc_oneshot_unit_handle_t *adc_handle; // 读取
    adc_cali_handle_t *cali_handle;        // 校准转化
};
#define ADC_NUM_MAX 6
static HAL_ADC_INFO adc_info_map[ADC_NUM_MAX];

static adc_cali_handle_t unit2_calibration_handle[4];
static adc_cali_handle_t unit1_calibration_handle[4];

static const char *TAG = "ADC";
static TaskHandle_t s_task_handle;

// ADC校准参考文档
// https://blog.csdn.net/chentuo2000/article/details/125448197
// https://blog.csdn.net/tianizimark/article/details/125348749
// https://blog.csdn.net/weixin_41995638/article/details/124190906

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel,
                                 adc_atten_t atten, adc_cali_handle_t *out_handle)
{
#define TAG "adc_calibration"
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = (adc_bitwidth_t)ADC_WIDTH_BITS,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    }
    else
    {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    // Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);
}

void SH_HAL::continuous_adc_init()
{
    adc_continuous_handle_t handle = NULL;
    static adc_channel_t channel[2] = {ADC_CHANNEL_2, ADC_CHANNEL_3};
    int channel_num = sizeof(channel) / sizeof(adc_channel_t);

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = 256,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 20 * 1000,
        .conv_mode = ADC_CONV_BOTH_UNIT,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++)
    {
        adc_pattern[i].atten = SH_ADC_ATTEN_11db;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = ADC_UNIT_1;
        adc_pattern[i].bit_width = ADC_WIDTH_BITS;

        ESP_LOGI(TAG, "adc_pattern[%d].atten is :%" PRIx8, i, adc_pattern[i].atten);
        ESP_LOGI(TAG, "adc_pattern[%d].channel is :%" PRIx8, i, adc_pattern[i].channel);
        ESP_LOGI(TAG, "adc_pattern[%d].unit is :%" PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));
}

void SH_HAL::oneshot_adc_init()
{
    static adc_oneshot_unit_handle_t unit1_handle;
    adc_oneshot_unit_init_cfg_t init1_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init1_config, &unit1_handle));

    // static adc_oneshot_unit_handle_t unit2_handle;
    // adc_oneshot_unit_init_cfg_t init2_config = {
    //     .unit_id = ADC_UNIT_2,
    // };
    // ESP_ERROR_CHECK(adc_oneshot_new_unit(&init2_config, &unit2_handle));

    // ESP_ERROR_CHECK(adc_oneshot_del_unit(unit2_handle)); // 去初始化

    adc_calibration_init(ADC_UNIT_1, (adc_channel_t)0, (adc_atten_t)SH_ADC_ATTEN_0db,
                         &unit1_calibration_handle[SH_ADC_ATTEN_0db]);
    adc_calibration_init(ADC_UNIT_1, (adc_channel_t)0, (adc_atten_t)SH_ADC_ATTEN_2_5db,
                         &unit1_calibration_handle[SH_ADC_ATTEN_2_5db]);
    adc_calibration_init(ADC_UNIT_1, (adc_channel_t)0, (adc_atten_t)SH_ADC_ATTEN_6db,
                         &unit1_calibration_handle[SH_ADC_ATTEN_6db]);
    adc_calibration_init(ADC_UNIT_1, (adc_channel_t)0, (adc_atten_t)SH_ADC_ATTEN_11db,
                         &unit1_calibration_handle[SH_ADC_ATTEN_11db]);

    // adc_calibration_init(ADC_UNIT_2, (adc_channel_t)0, (adc_atten_t)SH_ADC_ATTEN_0db,
    //                      &unit2_calibration_handle[SH_ADC_ATTEN_0db]);
    // adc_calibration_init(ADC_UNIT_2, (adc_channel_t)0, (adc_atten_t)SH_ADC_ATTEN_2_5db,
    //                      &unit2_calibration_handle[SH_ADC_ATTEN_2_5db]);
    // adc_calibration_init(ADC_UNIT_2, (adc_channel_t)0, (adc_atten_t)SH_ADC_ATTEN_6db,
    //                      &unit2_calibration_handle[SH_ADC_ATTEN_6db]);
    // adc_calibration_init(ADC_UNIT_2, (adc_channel_t)0, (adc_atten_t)SH_ADC_ATTEN_11db,
    //                      &unit2_calibration_handle[SH_ADC_ATTEN_11db]);

    // 初始化adc引脚对应通道等信息
    adc_info_map[POWER_ADC] = (HAL_ADC_INFO){ADC_CHANNEL_0, ADC_UNIT_1};
    adc_info_map[NTC_ADC] = (HAL_ADC_INFO){ADC_CHANNEL_1, ADC_UNIT_1};

    for (int ind = 0; ind < ADC_NUM_MAX; ++ind)
    {
        adc_info_map[ind].bitwidth = ADC_WIDTH_BITS;
        adc_info_map[ind].do_calibration = true;
        adc_info_map[ind].attenuation = SH_ADC_ATTEN_11db;
        if (adc_info_map[ind].unit_id == ADC_UNIT_1)
        {
            adc_info_map[ind].adc_handle = &unit1_handle;
            adc_info_map[ind].cali_handle =
                &unit1_calibration_handle[adc_info_map[ind].attenuation];
        }
        // else
        // {
        //     adc_info_map[ind].adc_handle = &unit2_handle;
        //     adc_info_map[ind].cali_handle =
        //         &unit2_calibration_handle[adc_info_map[ind].attenuation];
        // }
    }
}

bool SH_HAL::adcInit(uint8_t pinNum, SH_ADC_ATTEN attenuation)
{
    SH_HAL::gpioInit(pinNum, SH_HAL::SH_GPIO_MODE_INPUT,
                     SH_HAL::SH_GPIO_PULL_FLOATING, SH_HAL::SH_GPIO_LEVEL_UNDEF);

    adc_info_map[pinNum].attenuation = attenuation;
    if (adc_info_map[pinNum].unit_id == ADC_UNIT_1)
    {
        adc_info_map[pinNum].cali_handle =
            &unit1_calibration_handle[attenuation];
    }
    else
    {
        adc_info_map[pinNum].cali_handle =
            &unit2_calibration_handle[attenuation];
    }
    HAL_ADC_INFO *adc_info = &adc_info_map[pinNum];

    // 注释块的内容已在 halInit()中执行了
    //-------------ADC Init---------------//
    // adc_oneshot_unit_init_cfg_t init_config = {
    //     .unit_id = adc_info->unit_id,
    // };
    // ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_info->adc_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = (adc_atten_t)adc_info->attenuation,
        .bitwidth = (adc_bitwidth_t)adc_info->bitwidth,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc_info->adc_handle,
                                               adc_info->channel,
                                               &config));

    return true;
}

bool SH_HAL::adcSetAtten(uint8_t pinNum, SH_ADC_ATTEN attenuation)
{
    adcInit(pinNum, attenuation);
    // adc_info_map[pinNum].attenuation = attenuation;

    return true;
}

// uint16_t SH_HAL::adcRead(uint8_t pinNum, bool use_calibration)
// {
//     HAL_ADC_INFO *adc_info = &adc_info_map[pinNum];
//     int adc_raw;
//     ESP_ERROR_CHECK(adc_oneshot_read(*adc_info->adc_handle, adc_info->channel, &adc_raw));
//     if (!use_calibration)
//         return adc_raw;
//     // ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d",
//     //          adc_info->unit_id + 1, adc_info->channel, adc_raw);
//     int voltage;
//     if (use_calibration && adc_info->do_calibration)
//     {
//         ESP_ERROR_CHECK(adc_cali_raw_to_voltage(*adc_info->cali_handle, adc_raw, &voltage));
//         // ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV",
//         //          adc_info->unit_id + 1, adc_info->channel, voltage);
//     }

//     return voltage;
// }

#define DATA_BUF_LEN 20
static int adcData[DATA_BUF_LEN];

uint32_t SH_HAL::adcRead(uint8_t pinNum, bool use_calibration)
{
    HAL_ADC_INFO *adc_info = &adc_info_map[pinNum];
    bool doDelay = false;
    bool isFilter = true;
#define FILTER_NUM 12

    // 采集
    for (int pos = 0; pos < FILTER_NUM; pos += 1)
    {
        adc_oneshot_read(*adc_info->adc_handle, adc_info->channel, &adcData[pos]);
        // if (doDelay)
        // if (ADC_SOLDER_TEMP_PIN == pinNum)
        {
            // vTaskDelay(1 / portTICK_PERIOD_MS);
            esp_rom_delay_us(3);
        }
    }

    int filterNum = 6; // 过滤的数量,表示默认不过滤
    if (isFilter)
    {
        // 小到大排序
        uint32_t swap_tmp = 0;
        for (int i = 0; i < FILTER_NUM - 1; ++i)
        {
            for (int j = i + 1; j < FILTER_NUM; ++j)
            {
                if (adcData[i] > adcData[j])
                {
                    swap_tmp = adcData[i];
                    adcData[i] = adcData[j];
                    adcData[j] = swap_tmp;
                }
            }
        }
    }

    uint64_t sum = 0;
    for (int ind = 3; ind < FILTER_NUM - 3; ++ind)
    {
        sum += adcData[ind];
    }

    // if (ADC_SOLDER_TEMP_PIN == pinNum)
    // {
    //     SH_LOG("%d %d %d %d %d %d %d %d %d %d", adcData[0], adcData[1], adcData[2], adcData[3], adcData[4],
    //            adcData[5], adcData[6], adcData[7], adcData[8], adcData[9]);
    //     SH_LOG("avg = %d", sum / (FILTER_NUM - filterNum));
    // }

    if (!use_calibration)
    {
        // 目前几个接口都不使用未校准参数
        return sum / (FILTER_NUM - filterNum);
    }

    int voltage;
    if (use_calibration && adc_info->do_calibration)
    {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(*adc_info->cali_handle,
                                                sum / (FILTER_NUM - filterNum), &voltage));
        // ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV",
        //          adc_info->unit_id + 1, adc_info->channel, voltage);
    }

    return voltage;
}