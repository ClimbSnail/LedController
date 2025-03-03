#include "common.h"
#include "main.h"
#include "sh_hal/hal.h"
#include "freertos/FreeRTOS.h"

 #include<string.h>
#include "aes/esp_aes.h"
#include "mbedtls/aes.h"

// 激活变量的伪地址
uint32_t sysInfoModelActForgeAddr = 1072906225;

static string AESEncode(const char *plaintext)
{

// #define TEST_VER
#ifdef TEST_VER
    // AES在线加解密 http://tool.chacuo.net/cryptaes
    esp_aes_context aes_ctx;
    unsigned char key[24] = {0}; // 实际只能使用16位
    unsigned char cipher_text[64] = {0};
    unsigned char iv[32];

    // src的后几位是干扰字符
    unsigned char src_vi[45] = "MTIzNDU2Nzg5MDEyMzQ1ztFo";
    unsigned char src_key[33] = "QTc3MzFheDgxITrUEs";
    unsigned char src_vi_rear[25] = "NjExMTExMTExMjIyMjIyMjI=";
    unsigned char src_key_rear[19] = "g2MS46MDEyMzQ1Njc4";

    memcpy(&src_vi[44 - 24], src_vi_rear, 24);
    memcpy(&src_key[32 - 18], src_key_rear, 18);
    // this->run_log(NULL, "src_vi = %s\n", src_vi);
    // this->run_log(NULL, "src_key = %s\n", src_key);

    // 解码后的原始值如下 https://base64.us/
    // "A7731ax81!861.:012345678" base64编码后是 QTc3MzFheDgxITg2MS46MDEyMzQ1Njc4
    // "12345678901234561111111122222222" base64编码后是 MTIzNDU2Nzg5MDEyMzQ1NjExMTExMTExMjIyMjIyMjI=
    // memcpy((char *)key, "A7731ax81!861.:012345678", 24);
    // memcpy((char *)iv, "12345678901234561111111122222222", 16);

    base64_decode((unsigned char *)src_vi, sizeof(src_vi) - 1, iv, 32);
    base64_decode((unsigned char *)src_key, sizeof(src_key) - 1, key, 32);

    esp_aes_init(&aes_ctx);
    esp_aes_setkey(&aes_ctx, key, 128); // 只能是128

    esp_aes_crypt_cbc(&aes_ctx,
                      MBEDTLS_AES_ENCRYPT,
                      strlen(plaintext),
                      iv,
                      (const unsigned char *)plaintext,
                      cipher_text);

    // for (int loop = 0; loop < 64; loop++)
    // {
    //     SH_LOG("%02x ", cipher_text[loop]);
    // }
    // SH_LOG("end Print");
    string bs64_encode = base64_encode(cipher_text, 32);

    esp_aes_free(&aes_ctx);

    return bs64_encode;
#else
    return "";
#endif
}

static string SnailAESDecode(const char *cipher_text)
{
    // AES在线加解密 http://tool.chacuo.net/cryptaes
    esp_aes_context aes_ctx;
    unsigned char key[24] = {0}; // 实际只能使用16位
    unsigned char plain_text[64] = {0};
    unsigned char iv[32];

    // src的后几位是干扰字符
    unsigned char src_vi[45] = "MTIzNDU2Nzg5MDEyMzQ1ztFo";
    unsigned char src_key[33] = "QTc3MzFheDgxITrUEs";
    unsigned char src_vi_rear[25] = "NjExMTExMTExMjIyMjIyMjI=";
    unsigned char src_key_rear[19] = "g2MS46MDEyMzQ1Njc4";

    memcpy(&src_vi[44 - 24], src_vi_rear, 24);
    memcpy(&src_key[32 - 18], src_key_rear, 18);
    // this->run_log(NULL, "src_vi = %s\n", src_vi);
    // this->run_log(NULL, "src_key = %s\n", src_key);

    // 解码后的原始值如下 https://base64.us/
    // "A7731ax81!861.:012345678" base64编码后是 QTc3MzFheDgxITg2MS46MDEyMzQ1Njc4
    // "12345678901234561111111122222222" base64编码后是 MTIzNDU2Nzg5MDEyMzQ1NjExMTExMTExMjIyMjIyMjI=
    // memcpy((char *)key, "A7731ax81!861.:012345678", 24);
    // memcpy((char *)iv, "12345678901234561111111122222222", 16);

    base64_decode((unsigned char *)src_vi, sizeof(src_vi) - 1, iv, 32);
    // this->run_log(NULL, "src_vi = %s, sizeof(src_vi) = %d\n",
    //               (char *)src_vi, sizeof(src_vi) - 1);
    // this->run_log(NULL, "iv = %s\n", iv);
    // for (int loop = 0; loop < 32; loop += 2)
    // {
    //     this->run_log(NULL, "%c %0c ", iv[loop], iv[loop + 1]);
    // }

    base64_decode((unsigned char *)src_key, sizeof(src_key) - 1, key, 32);
    // this->run_log(NULL, "src_key = %s, sizeof(src_key) = %d\n",
    //               (char *)src_key, sizeof(src_key) - 1);
    // this->run_log(NULL, "key = %s\n", key);
    // for (int loop = 0; loop < 24; loop += 2)
    // {
    //     this->run_log(NULL, "%c %c ", key[loop], key[loop + 1]);
    // }

    esp_aes_init(&aes_ctx);
    esp_aes_setkey(&aes_ctx, key, 128); // 只能是128
    unsigned char bs64_decode[64] = {0};
    int bs64_decode_len = base64_decode(cipher_text, bs64_decode, 64);
    // SH_LOG("bs64_decode_len ---> %d", bs64_decode_len);
    // for (int loop = 0; loop < 64; loop++)
    // {
    //     SH_LOG("%02x ", bs64_decode[loop]);
    // }

    esp_aes_crypt_cbc(&aes_ctx,
                      MBEDTLS_AES_DECRYPT,
                      bs64_decode_len,
                      iv,
                      bs64_decode,
                      plain_text);

    esp_aes_free(&aes_ctx);

    return string((char *)plain_text);
}

static string generateNewInfo(const char *info_1, const char *info_2)
{
    // 通过合并两个信息生成新的信息
    char newNum[40] = {0};
    memset(newNum, 0, 40);
    int info_len_1 = strlen(info_1);
    int info_len_2 = strlen(info_2);
    int pos = 0;
    newNum[pos++] = info_len_1 / 10 + '0';
    newNum[pos++] = info_len_1 % 10 + '0';
    newNum[pos++] = info_len_2 / 10 + '0';
    newNum[pos++] = info_len_2 % 10 + '0';
    strcpy(&newNum[pos], info_1);
    pos += info_len_1;
    strcpy(&newNum[pos], info_2);
    pos += info_len_2;

#if SH_HARDWARE_VER == SH_ESP32S3_V26 || SH_HARDWARE_VER == SH_ESP32S2_WROOM_V27
    for (int cnt = pos; pos < 31; ++pos)
    {
        // 尾部使用0填充
        newNum[pos] = '0';
    }
    newNum[pos] = '1'; // 最后一位硬件版本号
#else
    for (int cnt = pos; pos < 32; ++pos)
    {
        // 尾部使用0填充
        newNum[pos] = '0';
    }
#endif

    return string(newNum);
}

static string getMcForInfo(const char *info)
{
    // 从信息中获取机器码
    if (0 == info[0])
    {
        return string("");
    }

    char mc_info[32] = {0};

    int mc_len = (info[0] - '0') * 10 + (info[1] - '0'); // 获取机器码的长度
    strncpy(mc_info, &info[4], mc_len);
    mc_info[4 + mc_len] = 0;

    return string(mc_info);
}

static string getQQForInfo(const char *info)
{
    // 从信息中获取机器码
    if (0 == info[0])
    {
        return string("");
    }

    char qq_info[32] = {0};

    int mc_len = (info[0] - '0') * 10 + (info[1] - '0');     // 获取机器码的长度
    int qq_num_len = (info[2] - '0') * 10 + (info[3] - '0'); // 获取qq号的长度
    strncpy(qq_info, &info[4 + mc_len], qq_num_len);
    qq_info[4 + mc_len + qq_num_len] = 0;

    return string(qq_info);
}

// string SnailManager::getMachineCode(void)
string getMachineCode(void)
{
    uint64_t num = ((~SH_HAL::getChipID()) & 0x0000FFFFFFFFFFFF) + 99;
    char code[32] = {0};
    snprintf(code, 32, "%llu", num);
    string tmp(code);
    return string("1") + tmp;
}

// bool SnailManager::activate(const char *sn)
// {
//     string newSN;
//     if (NULL != sn)
//     {
//         newSN = sn;
//     }
//     else
//     {
//         newSN = this->m_sysCoreConfigCache.sn;
//     }

// #ifdef TEST_VER
//     // 自动生成激活码
//     string newPlain = generateNewInfo(this->getMachineCode().c_str(), "773181861A");
//     SH_LOG("newPlain %s\n", newPlain.c_str());
//     string cipher_text = SnailAESEncode(newPlain.c_str());
//     SH_LOG("cipher_text %s\n", cipher_text.c_str());

//     newSN = cipher_text;
// #endif

//     // SH_LOG("sn: %s", newSN.c_str());
//     string plain_text = SnailAESDecode(newSN.c_str());
//     // SH_LOG("plain_text %s", plain_text.c_str());
//     // 反解出来的机器码
//     string decode_machineCode = getMcForInfo(plain_text.c_str());
//     // SH_LOG("decode_machineCode %s\n\n", decode_machineCode.c_str());

//     if (decode_machineCode == getMachineCode())
//     {
//         this->m_sysCoreConfigCache.activateState = 1;
//         // 防止汇编找到变量绕过，使用计算得到地址混淆参数
//         // sysInfoModel.coreConfig.activateState = m_sysCoreConfigCache.activateState;
//         MODEL_ACTIVE_PARAM = m_sysCoreConfigCache.activateState;
// #ifdef TEST_VER
//         {
//             this->m_sysCoreConfigCache.sn = newSN;
// #else
//         if (NULL != sn)
//         {
//             this->m_sysCoreConfigCache.sn = sn;
// #endif
//             // 持久化新的SN
//             save_settings_type = AUTO_SAVE_TYPE_MANAGER;
//             xQueueSend(auto_save_settings_que, &save_settings_type, NULL);
//         }

//         return true;
//     }
//     else
//     {
//         // 将无效的sn删除
//         this->m_sysCoreConfigCache.activateState = 0;
//         // 防止汇编找到变量绕过，使用计算得到地址混淆参数
//         // sysInfoModel.coreConfig.activateState = m_sysCoreConfigCache.activateState;
//         MODEL_ACTIVE_PARAM = m_sysCoreConfigCache.activateState;
//         if (NULL == sn)
//         {
//             // 使用保存的密钥认证 但又认证失败
//             this->m_sysCoreConfigCache.sn = "";
//             // 持久化新的SN
//             save_settings_type = AUTO_SAVE_TYPE_MANAGER;
//             xQueueSend(auto_save_settings_que, &save_settings_type, NULL);
//         }
//     }

//     return false;
// }