/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "sh_hal/hal.h"
#include "common.h"
#include "config.h"

#include "controller/LedController/LedController.h"
#include "controller/networker/networker.h"
#include "controller/networker/sh_mqtt.h"

TaskHandle_t handleTaskSaveConfig;
void TaskSaveConfig(void *parameter);

TaskHandle_t handleTaskNetworker;
void TaskBackstage(void *parameter);

bool StartUpCheck();
bool Shutdown();

// SnailManager *manager = new SnailManager(true);

// LED灯控制器
LedController *ledCtrl = new LedController(LED_CTRL_NAME, NULL,
                                           LED_PWM_0, LED_PWM_1,
                                           LED_PWM_2, LED_PWM_3,
                                           FAN_PWM);

Networker *networker = new Networker(NETWORKER_CTRL_NAME, NULL);

extern "C" void app_main(void)
{
    SH_HAL::halInit();
    // vTaskDelay(1500);

    if (!StartUpCheck())
    {
        vTaskDelay(500);
        vTaskDelete(NULL);
    }

    // 需要放在Setup里初始化
    if (!g_flashCfg.init())
    {
        SH_LOG("SPIFFS Mount Failed");
        return;
    }
    else
    {
        SH_LOG("SPIFFS Mount Succ");
    }

    // mqtt消息队列需要在使用之前初始化
    mqttMsgQue = xQueueCreate(10, sizeof(unsigned char));

    ledCtrl->Start();
    networker->Start();
    // 设置web数据信息指针
    networker->SetCtrlCfgPoint(&(ledCtrl->m_utilConfigCache));
    // 默认关闭web配置页面
    networker->m_utilConfigCache.webServerEnable = ENABLE_STATE_CLOSE;

    // 检测开机状态意图
    int startupOpaCnt = 0;
    for (startupOpaCnt = 0; true || startupOpaCnt < 10; ++startupOpaCnt)
    {
        vTaskDelay(700);
        ledCtrl->IndicateLite();
        if (SH_HAL::SH_GPIO_LEVEL_LOW != SH_HAL::gpioLevelRead(KEY_SW))
        {
            break;
        }
    }
    if (10 <= startupOpaCnt)
    {
        SH_LOG("LedController Reset...");
        ledCtrl->Reset();
        Shutdown();
    }
    else if (6 <= startupOpaCnt)
    {
        SH_LOG("LedController Config...");
        // web配置页面使能标志位
        networker->m_utilConfigCache.webServerEnable = ENABLE_STATE_OPEN;
    }
    else if (3 <= startupOpaCnt)
    {
        SH_LOG("LedController Config...");
        // web配置页面使能标志位
        networker->m_utilConfigCache.webServerEnable = ENABLE_STATE_OPEN;
        ledCtrl->ResetBrightness(); // 复位亮度
    }

    interior_ntc.init();
    // 正式生效
    ledCtrl->Application();

    // // 风扇
    // SH_HAL::gpioInit(FAN_PWM, SH_HAL::SH_GPIO_MODE_OUTPUT, SH_HAL::SH_GPIO_PULL_DOWN_ONLY);
    // SH_HAL::pwmInit(FAN_PWM, PWM_GROUP_2_FREQ, PWM_GROUP_2_BIT_WIDTH);

    // 配置保存任务
    BaseType_t taskSaveConfigReturned = xTaskCreatePinnedToCore(
        TaskSaveConfig,
        "TaskSaveConfig",
        6 * 512,
        nullptr,
        TASK_MANAGER_BG_PRIORITY,
        &handleTaskSaveConfig,
        TASK_MANAGER_BG_CORE_ID);
    if (taskSaveConfigReturned != pdPASS)
    {
        SH_LOG("Fail Create TaskSaveConfig !!!");
    }
    else
    {
        SH_LOG("Succ Create TaskSaveConfig !!!");
    }

    knobs.Start();
    vTaskDelay(500);

    // 后台任务
    BaseType_t taskBackstageReturned = xTaskCreatePinnedToCore(
        TaskBackstage,
        "TaskBackstage",
        6 * 512,
        nullptr,
        TASK_MANAGER_BG_PRIORITY,
        &handleTaskNetworker,
        TASK_MANAGER_BG_CORE_ID);
    if (taskBackstageReturned != pdPASS)
    {
        SH_LOG("Fail Create TaskBackstage !!!");
    }
    else
    {
        SH_LOG("Succ Create TaskBackstage !!!");
    }

    while (true)
    {
        networker->Process();

        vTaskDelay(10);
    }

    vTaskDelete(NULL);
}

bool Shutdown()
{
    // 关机
    SH_LOG("Shutdown !!!");
    ledCtrl->End();
    SH_HAL::gpioLevelSet(POWER_EN, SH_HAL::SH_GPIO_LEVEL_LOW);
    SH_HAL::gpioLevelSet(WORK_LED, SH_HAL::SH_GPIO_LEVEL_LOW);
    vTaskDelay(1000);
    SH_LOG("Shutdown Finished !!!!"); // 已经关机 所以运行不到这一条代码
    vTaskDelete(NULL);
    return true;
}

void TaskSaveConfig(void *parameter)
{
    float temperature = 25.0f;
    vTaskDelay(60); // 适当增加延时，消除初始画面为空时的白屏问题
    for (;;)
    {
        vTaskDelay(5000);

        ledCtrl->UpdateConfig();
    }
}

void TaskBackstage(void *parameter)
{
    vTaskDelay(60); // 适当增加延时，消除初始画面为空时的白屏问题
    unsigned char queParam = MQTT_MSG_TYPE_UNKNOWN;

    for (;;)
    {
        // networker->Process();

        vTaskDelay(10);
        ledCtrl->Process();

        KeyInfo keyInfo = knobs.get_data();
        if (KNOBS_STATE_PRESS_LOST_LONG_LONG == keyInfo.switch_status)
        {
            Shutdown();
            break;
        }
        ledCtrl->Operate(keyInfo.switch_status, keyInfo.pulse_count * 1);

        if (xQueueReceive(mqttMsgQue, &queParam, (TickType_t)5))
        {
            if (MQTT_MSG_TYPE_CLOSE_LED == queParam)
            {
                ledCtrl->Close();
                ledCtrl->m_enableFlag = ENABLE_STATE_CLOSE;
            }
            else if (MQTT_MSG_TYPE_OPEN_LED == queParam)
            {
                ledCtrl->Open();
                ledCtrl->m_enableFlag = ENABLE_STATE_OPEN;
            }
        }
        // static int saveFlagCnt = 0;
        // ++saveFlagCnt;
        // if (0 == saveFlagCnt % 200)
        // {
        //     ledCtrl->UpdateConfig();
        // }
    }
}

bool StartUpCheck()
{
    SH_HAL::gpioInit(KEY_SW, SH_HAL::SH_GPIO_MODE_INPUT, SH_HAL::SH_GPIO_PULL_UP_ONLY);
    SH_HAL::gpioInit(POWER_EN, SH_HAL::SH_GPIO_MODE_OUTPUT, SH_HAL::SH_GPIO_PULL_DOWN_ONLY);
    SH_HAL::gpioInit(WORK_LED, SH_HAL::SH_GPIO_MODE_OUTPUT, SH_HAL::SH_GPIO_PULL_DOWN_ONLY);

    SH_HAL::gpioLevelSet(POWER_EN, SH_HAL::SH_GPIO_LEVEL_LOW);
    SH_HAL::gpioLevelSet(WORK_LED, SH_HAL::SH_GPIO_LEVEL_LOW);
    vTaskDelay(5);

    if (SH_HAL::SH_GPIO_LEVEL_LOW == SH_HAL::gpioLevelRead(KEY_SW))
    {
        vTaskDelay(10);
        if (SH_HAL::SH_GPIO_LEVEL_LOW == SH_HAL::gpioLevelRead(KEY_SW))
        {
            SH_HAL::gpioLevelSet(POWER_EN, SH_HAL::SH_GPIO_LEVEL_HIGH);
            SH_HAL::gpioLevelSet(WORK_LED, SH_HAL::SH_GPIO_LEVEL_HIGH);
            return true;
        }
    }

    return false;
}