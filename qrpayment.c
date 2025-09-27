#include <stdio.h>
#include <string.h>
#include <stdlib.h>   // สำหรับ malloc()
#include <stdint.h>   // สำหรับ uint16_t, uint8_t
#include <stdio.h>
#include "qrpayment.h"

// ------------------- ฟังก์ชัน CRC16 -------------------
uint16_t crc16_ccitt(const char *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint8_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc & 0xFFFF;
}

// ------------------- สร้าง PromptPay QR -------------------
char* build_promptpay_qr(const char *phone, const char *amount) {
    char merchantInfo[128];
    char payload[512];
    char phoneProxy[32];

    if (phone[0] == '0')
        sprintf(phoneProxy, "0066%s", phone + 1);
    else
        sprintf(phoneProxy, "0066%s", phone);

    sprintf(merchantInfo,
        "0016A000000677010111"
        "0113%s", phoneProxy
    );

    sprintf(payload,
        "000201"
        "010212"
        "29%02zu%s"
        "5802TH"
        "5303764"
        "54%02zu%s"
        "6304",
        strlen(merchantInfo), merchantInfo,
        strlen(amount), amount
    );

    uint16_t crc = crc16_ccitt(payload, strlen(payload));
    char *output = (char*)malloc(strlen(payload) + 5);
    if (!output) return NULL;

    sprintf(output, "%s%04X", payload, crc);
    return output;
}
