#include "flash_fs.h"

#include "common.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "SPIFFS";
#define FILE_WRITE "w"
#define FILE_READ "r"

char dataCache[1536];

/* You only need to format SPIFFS the first time you run a
   test or else use the LITTLEFS plugin to create a partition
   https://github.com/lorol/arduino-esp32littlefs-plugin */

#define FORMAT_LITTLEFS_IF_FAILED true

FlashFS::FlashFS()
{
    total = 0;
    used = 0;
}

bool FlashFS::init(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs", // spiffs backgroud
        .partition_label = "spiffs",
        .max_files = 5,
        .format_if_mount_failed = true};

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }

#ifdef CONFIG_EXAMPLE_SPIFFS_CHECK_ON_START
    ESP_LOGI(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return false;
    }
    else
    {
        ESP_LOGI(TAG, "SPIFFS_check() successful");
    }
#endif

    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return false;
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Check consistency of reported partiton size info.
    if (used > total)
    {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return false;
        }
        else
        {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }

    return true;
}

FlashFS::~FlashFS()
{
}

uint16_t FlashFS::readFile(const char *path, uint8_t *info)
{
    SH_LOG("Reading file: %s\r\n", path);

    FILE *file = fopen(path, FILE_READ);
    uint16_t ret_len = 0;
    if (file == NULL)
    {
        SH_LOGE("Failed to open file for reading: %s", path);
        return ret_len;
    }

    while (!feof(file))
    {
        ret_len += fread(info + ret_len, sizeof(uint8_t), 15, file);
    }
    fclose(file);

    return ret_len;
}

void FlashFS::writeFile(const char *path, const char *message)
{
    SH_LOG("Writing file: %s\r\n", path);

    FILE *file = fopen(path, FILE_WRITE);
    if (!file)
    {
        SH_LOGE("- failed to open file for writing\n");
        return;
    }

    if (fprintf(file, message))
    {
        SH_LOG("- file written\n");
    }
    else
    {
        SH_LOGE("- write failed\n");
    }

    fclose(file);
}

void FlashFS::renameFile(const char *src, const char *dst)
{
    // 未改成全路径
    SH_LOG("Renaming file %s to %s\r\n", src, dst);

    if (0 == rename(src, dst))
    {
        SH_LOG("- file renamed");
    }
    else
    {
        SH_LOGE("- rename failed");
    }
}

void FlashFS::deleteFile(const char *path)
{
    if (0 == remove(path))
    {
        SH_LOG("- deleted %s succ\r\n ", path);
    }
    else
    {
        SH_LOGE("- deleted %s failed\r\n", path);
    }
}

void FlashFS::deleteFileByPrefix(const char *pathPrefix)
{
    // 实现前缀删除
    SH_LOG("Deleting file by prefix: %s\r\n", pathPrefix);

    DIR *dir = opendir(ROOT_DIR);
    if (dir == NULL)
        return;

    char full_path[48];
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        snprintf(full_path, sizeof(full_path) - 1, "%s/%s", ROOT_DIR, entry->d_name);
        if (entry->d_type != DT_DIR)
        {
            // SH_LOG("文件: %s", full_path);
            if (strncmp(full_path, pathPrefix, strlen(pathPrefix)) == 0)
            {
                // 相等 匹配成功
                if (0 == remove(full_path))
                {
                    SH_LOG("- deleted %s succ\r\n", full_path);
                }
                else
                {
                    SH_LOGE("- deleted %s failed\r\n", full_path);
                }
            }
        }
        else
        {
            SH_LOG("目录: /%s", full_path);
        }
    }
    closedir(dir);
}

bool analyseParam(char *info, int argc, char **argv)
{
    int cnt; // 记录解析到第几个参数
    for (cnt = 0; cnt < argc; ++cnt)
    {
        argv[cnt] = info;
        while (*info != '\n')
        {
            ++info;
        }
        *info = 0;
        ++info;
    }
    return true;
}