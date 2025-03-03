
#include "sh_hal/hal.h"
#include "config.h"
#include <inttypes.h>
#include "esp_mac.h"
#include "esp_efuse.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "soc/soc_caps.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_continuous.h"

#include "driver/uart.h"

#include "esp_timer.h"
#include "driver/timer.h"
// #include "driver/gptimer.h"

#include "driver/touch_pad.h"
#include "common.h"
#include "math.h"

#define ESP_INTR_FLAG_DEFAULT 0
#define ECHO_UART_PORT_NUM 1

// pwm or adc的通道记录
uint8_t gpio_pwm_chan_map[GPIO_NUM_MAX] = {0};
struct CHANNEL_INFO
{
    bool enable;
    ledc_channel_config_t chan_config;
    ledc_timer_config_t timer_config;
};
CHANNEL_INFO pwm_chan_info[8] = {0};

uint32_t SH_HAL::getSysFreq()
{
    return 240;
}

uint64_t SH_HAL::getChipID()
{
    uint64_t _chipmacid = 0LL;
    esp_efuse_mac_get_default((uint8_t *)(&_chipmacid));
    return _chipmacid;
}

// 过零检测
#define ZERO_CHECK_MAX 4
struct INTR_CB_INFO
{
    void (*cb)(void *);
    void *param;
};
static INTR_CB_INFO zeroCheckInfo[ZERO_CHECK_MAX];
static uint8_t zeroCheckIndex = 0;

bool SH_HAL::halInit(void)
{
    // 初始化pwm对应的通道关系
    gpio_pwm_chan_map[LED_PWM_0] = PWM_0_CHANNEL;
    gpio_pwm_chan_map[LED_PWM_1] = PWM_1_CHANNEL;
    gpio_pwm_chan_map[LED_PWM_2] = PWM_2_CHANNEL;
    gpio_pwm_chan_map[LED_PWM_3] = PWM_3_CHANNEL;
    gpio_pwm_chan_map[FAN_PWM] = FAN_CHANNEL;

    for (int ind = 0; ind < 8; ++ind)
    {
        pwm_chan_info[ind].enable = false;
    }

    oneshot_adc_init();
    // continuous_adc_init();

    // 开启gpio中断服务
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);

    // 初始化过零检测参数
    for (int ind = 0; ind < ZERO_CHECK_MAX; ++ind)
    {
        zeroCheckInfo[ind].cb = NULL;
        zeroCheckInfo[ind].param = NULL;
    }

    return true;
}

bool SH_HAL::gpioInit(uint8_t pinNum, SH_GPIO_MODE mode,
                      SH_GPIO_PULL pullMode, SH_GPIO_LEVEL initLevel)
{
    gpio_config_t io_conf = {
        // .pin_bit_mask = GPIO_OUTPUT_PIN_SEL,
        .pin_bit_mask = (1ULL << pinNum),
        .mode = (gpio_mode_t)mode,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    // SH_LOG("SH_HAL::gpioInit GPIO[%u] mode[%u] pullMode[%u] initLevel[%u]",
    //        pinNum, mode, pullMode, initLevel);

    if (SH_GPIO_PULL::SH_GPIO_PULL_UP_ONLY == pullMode ||
        SH_GPIO_PULL::SH_GPIO_PULL_UP_DOWN == pullMode)
    {
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    }

    if (SH_GPIO_PULL::SH_GPIO_PULL_DOWN_ONLY == mode ||
        SH_GPIO_PULL::SH_GPIO_PULL_UP_DOWN == mode)
    {
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    }
    gpio_config(&io_conf);

    if (SH_GPIO_LEVEL_UNDEF != initLevel)
    {
        gpioLevelSet(pinNum, initLevel);
    }

    return true;
}
bool SH_HAL::gpioLevelSet(uint8_t pinNum, SH_GPIO_LEVEL level)
{
    gpio_set_level((gpio_num_t)pinNum, level == SH_GPIO_LEVEL::SH_GPIO_LEVEL_LOW ? 0 : 1);
    // 增强输出能力
    // gpio_set_drive_capability((gpio_num_t)pinNum, GPIO_DRIVE_CAP_3);
    return true;
}

uint8_t SH_HAL::gpioLevelRead(uint8_t pinNum)
{
    return gpio_get_level((gpio_num_t)pinNum);
}

bool SH_HAL::gpioSetIntr(uint8_t pinNum, void (*gpio_isr_handler)(void *),
                         SH_GPIO_INTR_TYPE interruptType, void *param)
{
    // SH_LOG("SH_HAL::gpioSetIntr GPIO[%u]", pinNum);

    // // 按键对应的GPIO复位
    // gpio_reset_pin((gpio_num_t)pinNum);
    // // 设置按键的GPIO为输入
    // gpio_set_direction((gpio_num_t)pinNum, gpioMode);
    // gpio_set_pull_mode((gpio_num_t)pinNum, pullMode);
    // change gpio interrupt type for one pin
    gpio_set_intr_type((gpio_num_t)pinNum, (gpio_int_type_t)interruptType);

    // gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    // // install gpio isr service
    // gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    // hook isr handler for specific gpio pin
    gpio_isr_handler_add((gpio_num_t)pinNum, gpio_isr_handler, param);

    return 0;
}

bool SH_HAL::gpioDisableIntr(uint8_t pinNum)
{
    // remove isr handler for gpio number.
    // gpio_intr_enable((gpio_num_t)pinNum);
    gpio_isr_handler_remove((gpio_num_t)pinNum);
    return true;
}

bool SH_HAL::pwmTest(void)
{
    SH_LOG("Buzzer start 1524");
    uint32_t freq = SH_HAL::pwmInit(LED_PWM_0, PWM_GROUP_0_FREQ << 7, PWM_GROUP_0_BIT_WIDTH);
    SH_HAL::pwmSetDuty(LED_PWM_0, 1524);
    vTaskDelay(5000);

    SH_LOG("Buzzer stop");
    SH_HAL::pwmEnable(LED_PWM_0, false, SH_HAL::SH_GPIO_LEVEL_LOW);
    vTaskDelay(5000);

    SH_LOG("Buzzer 524");
    SH_HAL::pwmSetDuty(LED_PWM_0, 524);
    vTaskDelay(5000);

    SH_LOG("Buzzer start");
    SH_HAL::pwmEnable(LED_PWM_0, true, SH_HAL::SH_GPIO_LEVEL_LOW);
    vTaskDelay(5000);

    return 0;
}

uint32_t SH_HAL::pwmInit(uint8_t pinNum, uint32_t freq, uint8_t bitNum)
{
    uint8_t channel = gpio_pwm_chan_map[pinNum];
    // 每两个通道将会公用一个pwm定时器
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        // .speed_mode = LEDC_HIGH_SPEED_MODE, // esp32-c3 不支持高速模式
        .duty_resolution = (ledc_timer_bit_t)bitNum,
        .timer_num = (ledc_timer_t)(channel / 2),
        .freq_hz = freq, // Set output frequency at 4 kHz
        .clk_cfg = LEDC_AUTO_CLK};
    // .clk_cfg = LEDC_USE_XTAL_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .gpio_num = pinNum,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        // .speed_mode = LEDC_HIGH_SPEED_MODE, // esp32-c3 不支持高速模式
        .channel = (ledc_channel_t)channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = (ledc_timer_t)(channel / 2),
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // 保存通道的配置信息
    pwm_chan_info[channel].enable = true;
    pwm_chan_info[channel].timer_config = ledc_timer;
    pwm_chan_info[channel].chan_config = ledc_channel;

    // SH_LOG("speed_mode = %u", pwm_chan_info[channel].chan_config.speed_mode);

    return ledc_get_freq(pwm_chan_info[channel].chan_config.speed_mode,
                         (ledc_timer_t)(channel / 2));
}

bool SH_HAL::pwmEnable(uint8_t pinNum, bool enable, SH_GPIO_LEVEL idleLevel)
{
    if (true == enable)
    {
        ledc_channel_config_t *channel_cfg = &(pwm_chan_info[gpio_pwm_chan_map[pinNum]].chan_config);
        if (true == pwm_chan_info[gpio_pwm_chan_map[pinNum]].enable)
        {
            if (SH_GPIO_LEVEL_LOW == idleLevel)
            {
                channel_cfg->duty = 0;
            }
            else if (SH_GPIO_LEVEL_HIGH == idleLevel)
            {
                ledc_timer_config_t *timer_cfg = &(pwm_chan_info[gpio_pwm_chan_map[pinNum]].timer_config);
                channel_cfg->duty = pow(2, (uint8_t)timer_cfg->duty_resolution); // - 1;
            }
            ESP_ERROR_CHECK(ledc_channel_config(channel_cfg));
        }
        else
        {
            SH_LOG("Err pwmEnable == false GPIO[%u]", pinNum);
        }
    }
    else
    {
        uint8_t channel = gpio_pwm_chan_map[pinNum];
        // 停止PWM输出并断开GPIO的PWM通道
        ESP_ERROR_CHECK(ledc_stop(pwm_chan_info[channel].chan_config.speed_mode,
                                  (ledc_channel_t)channel, idleLevel));
    }
    return true;
}

bool SH_HAL::pwmSetDuty(uint8_t pinNum, uint32_t duty)
{
    uint8_t channel = gpio_pwm_chan_map[pinNum];
    // 执行本函数，不管是否关闭pwm，都会重新启动
    pwm_chan_info[channel].chan_config.duty = duty;

    ESP_ERROR_CHECK(ledc_set_duty(pwm_chan_info[channel].chan_config.speed_mode,
                                  (ledc_channel_t)channel,
                                  duty));

    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(pwm_chan_info[channel].chan_config.speed_mode,
                                     (ledc_channel_t)channel));

    return true;
}

uint32_t SH_HAL::pwmSetFreq(uint8_t pinNum, uint32_t freq)
{
    uint8_t channel = gpio_pwm_chan_map[pinNum];
    // 设置pwm频率 如果是很高或者很低的频率 可以考虑修改速度模式
    ESP_ERROR_CHECK(ledc_set_freq(pwm_chan_info[channel].chan_config.speed_mode,
                                  (ledc_timer_t)(pwm_chan_info[channel].chan_config.timer_sel),
                                  freq));

    return ledc_get_freq(pwm_chan_info[channel].chan_config.speed_mode,
                         (ledc_timer_t)(pwm_chan_info[channel].chan_config.timer_sel));
}

// #define TOUCH_BUTTON_NUM 14
// #define TOUCH_CHANGE_CONFIG 0

// void SH_HAL::touchInit()
// {
//     touch_pad_init();

//     touch_pad_config(TOUCH_PIN);
// #if TOUCH_CHANGE_CONFIG
//     /* If you want change the touch sensor default setting, please write here(after initialize). There are examples: */
//     touch_pad_set_measurement_interval(TOUCH_PAD_SLEEP_CYCLE_DEFAULT);
//     touch_pad_set_charge_discharge_times(TOUCH_PAD_MEASURE_CYCLE_DEFAULT);
//     touch_pad_set_voltage(TOUCH_PAD_HIGH_VOLTAGE_THRESHOLD, TOUCH_PAD_LOW_VOLTAGE_THRESHOLD, TOUCH_PAD_ATTEN_VOLTAGE_THRESHOLD);
//     touch_pad_set_idle_channel_connect(TOUCH_PAD_IDLE_CH_CONNECT_DEFAULT);
//     for (int i = 0; i < TOUCH_BUTTON_NUM; i++)
//     {
//         touch_pad_set_cnt_mode(button[i], TOUCH_PAD_SLOPE_DEFAULT, TOUCH_PAD_TIE_OPT_DEFAULT);
//     }
// #endif
//     /* Denoise setting at TouchSensor 0. */
//     touch_pad_denoise_t denoise = {
//         /* The bits to be cancelled are determined according to the noise level. */
//         .grade = TOUCH_PAD_DENOISE_BIT4,
//         .cap_level = TOUCH_PAD_DENOISE_CAP_L4,
//     };
//     touch_pad_denoise_set_config(&denoise);
//     touch_pad_denoise_enable();

//     /* Enable touch sensor clock. Work mode is "timer trigger". */
//     touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
//     touch_pad_fsm_start();
// }

// uint32_t SH_HAL::touchRead()
// {
//     uint32_t touch_value;
//     // If open the filter mode, please use this API to get the touch pad count.
//     touch_pad_read_raw_data(TOUCH_PIN, &touch_value);

//     return touch_value;
// }

#if UART_BAUD != 115200
#define USE_MY_UART
static bool s_uartInited = false;

// #define CONFIG_TINYUSB_CDC_RX_BUFSIZE 512
static const char *TAG = "Tinyusb example";
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "driver/usb_serial_jtag.h"
static uint8_t tinyusb_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];
static uint16_t tinyusbBufLen = 0;
tinyusb_cdcacm_itf_t g_cdc_port = TINYUSB_CDC_ACM_0;
void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
    /* initialization */
    size_t rx_size = 0;

    /* read */
    esp_err_t ret = tinyusb_cdcacm_read((tinyusb_cdcacm_itf_t)itf, &tinyusb_buf[tinyusbBufLen],
                                        CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Data from channel %d:", (tinyusb_cdcacm_itf_t)itf);
        ESP_LOG_BUFFER_HEXDUMP(TAG, tinyusb_buf, rx_size, ESP_LOG_INFO);
    }
    else
    {
        ESP_LOGE(TAG, "Read error");
    }

    // /* write back */
    // tinyusb_cdcacm_write_queue((tinyusb_cdcacm_itf_t)itf,
    //                            &tinyusb_buf[tinyusbBufLen], rx_size);
    // tinyusb_cdcacm_write_flush((tinyusb_cdcacm_itf_t)itf, 0);

    if (ret == ESP_OK)
    {
        tinyusbBufLen += rx_size;
    }
}

#include "soc/rtc_cntl_reg.h"
static int s_prev_rts_state = 0;

void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
    int dtr = event->line_state_changed_data.dtr;
    int rts = event->line_state_changed_data.rts;
    // ESP_LOGI(TAG, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);
    // char sent[32] = "";
    // snprintf(sent, 32, "DTR:%d, RTS:%d\n", dtr, rts);
    // size_t tx_size = tinyusb_cdcacm_write_queue(g_cdc_port, (uint8_t *)sent, 14);
    // tinyusb_cdcacm_write_flush(g_cdc_port, 0);

    // if (!rts && s_prev_rts_state)
    // {
    //     if (dtr)
    //     {
    //         snprintf(sent, 32, "reboot\n", dtr, rts);
    //         size_t tx_size = tinyusb_cdcacm_write_queue(g_cdc_port, (uint8_t *)sent, 10);
    //         tinyusb_cdcacm_write_flush(g_cdc_port, 0);
    //         // REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
    //         // REG_WRITE(RTC_CNTL_OPTIONS0_REG, RTC_CNTL_SW_SYS_RST);
    //         // // abort();
    //         // esp_restart();
    //     }
    //     else
    //     {
    //         // s_queue_reboot = REBOOT_NORMAL;
    //     }
    // }
    // s_prev_rts_state = rts;
}
#endif

bool SH_HAL::uartInit(uint32_t baud)
{
#ifdef USE_MY_UART
#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_PID (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                 _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4))

#define USB_VID 0xCafe
#define USB_BCD 0x0200
    if (true == s_uartInited)
    {
        return 0;
    }

    ESP_LOGI(TAG, "USB initialization");
    // tusb_desc_device_t const desc_device =
    //     {
    //         .bLength = sizeof(tusb_desc_device_t),
    //         .bDescriptorType = TUSB_DESC_DEVICE,
    //         .bcdUSB = USB_BCD,
    //         .bDeviceClass = 0x00,
    //         .bDeviceSubClass = 0x00,
    //         .bDeviceProtocol = 0x00,
    //         .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    //         .idVendor = USB_VID,
    //         .idProduct = USB_PID,
    //         .bcdDevice = 0x0100,

    //         .iManufacturer = 0x01,
    //         .iProduct = 0x02,
    //         .iSerialNumber = 0x03,

    //         .bNumConfigurations = 0x01};

    const tinyusb_config_t tusb_cfg = {
        // .device_descriptor = &desc_device,
        .device_descriptor = NULL,
        .string_descriptor = NULL,
        .external_phy = false,
        .configuration_descriptor = NULL,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = g_cdc_port,
        .rx_unread_buf_sz = 64,
        .callback_rx = &tinyusb_cdc_rx_callback, // the first way to register a callback
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL};

    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    /* the second way to register a callback */
    ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
        g_cdc_port,
        CDC_EVENT_LINE_STATE_CHANGED,
        &tinyusb_cdc_line_state_changed_callback));

    ESP_LOGI(TAG, "USB initialization DONE");

    s_uartInited = true;
#endif
    return 0;
}

int SH_HAL::uartAvailable()
{
#ifdef USE_MY_UART
    if (false == s_uartInited)
    {
        return 0;
    }
    return tinyusbBufLen;
#endif
    return 0;
}

size_t SH_HAL::uartRead(uint8_t *data, size_t size, uint32_t timeout)
{
#ifdef USE_MY_UART
    if (false == s_uartInited)
    {
        return 0;
    }
    uint16_t read_size = size > tinyusbBufLen ? tinyusbBufLen : size;
    memcpy(data, tinyusb_buf, read_size);
    tinyusbBufLen -= read_size;
    memcpy(tinyusb_buf, tinyusb_buf + read_size, tinyusbBufLen);
    return read_size;
#endif
    return 0;
}

size_t SH_HAL::uartWrite(const uint8_t *data, size_t size)
{
#ifdef USE_MY_UART
    if (false == s_uartInited)
    {
        return 0;
    }
    /* write back */
    size_t tx_size = tinyusb_cdcacm_write_queue(g_cdc_port, data, size);
    tinyusb_cdcacm_write_flush(g_cdc_port, 0);
    return tx_size;
#endif
    return 0;
}
