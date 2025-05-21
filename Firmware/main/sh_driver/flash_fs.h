#ifndef FLASH_FS_H
#define FLASH_FS_H

#include <stdint.h>
#include <cstddef>

#define ROOT_DIR "/spiffs"

extern char dataCache[1536];

class FlashFS
{
public:
    FlashFS();

    ~FlashFS();

    bool init(void);

    // void createDir(const char *path);

    // void removeDir(const char *path);

    uint16_t readFile(const char *path, uint8_t *info);

    void writeFile(const char *path, const char *message);

    void renameFile(const char *src, const char *dst);

    void deleteFile(const char *path);

    void deleteFileByPrefix(const char *pathPrefix);

    // // 写入到多级递归文件路径中
    // void writeFile2(const char *path, const char *message);

    // //删除多级递归文件
    // void deleteFile2(const char *path);

    size_t total = 0;
    size_t used = 0;
};

bool analyseParam(char *info, int argc, char **argv);

#endif