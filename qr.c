#include <stdio.h>
#include <string.h>
#include <stdint.h>

// คำนวณ CRC16-CCITT (poly=0x1021, init=0xFFFF)
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

void build_promptpay_qr(const char *phone, const char *amount, char *output) {
    char merchantInfo[128];
    char payload[512];

    // ฟอร์แมต Proxy = เบอร์โทร แบบ 0066XXXXXXXXX
    // ตัดเลข 0 ตัวหน้าออกแล้วเติม 0066 แทน
    char phoneProxy[32];
    if (phone[0] == '0') {
        sprintf(phoneProxy, "0066%s", phone + 1);
    } else {
        sprintf(phoneProxy, "0066%s", phone);
    }

    // Merchant Account Information (ID 29)
    sprintf(merchantInfo,
        "0016A000000677010111"   // AID ของ PromptPay
        "0113%s", phoneProxy     // Proxy (เบอร์)
    );

    // สร้าง payload (ยังไม่รวม CRC)
    sprintf(payload,
        "000201"             // Payload Format Indicator
        "010212"             // Point of Initiation Method (12 = dynamic)
        "29%02zu%s"          // Merchant Account Info
        "5802TH"             // Country Code
        "5303764"            // Currency (THB)
        "54%02zu%s"          // Transaction Amount
        "6304",              // CRC placeholder
        strlen(merchantInfo), merchantInfo,
        strlen(amount), amount
    );

    // คำนวณ CRC จาก payload ที่มี "6304" ต่อท้าย
    uint16_t crc = crc16_ccitt(payload, strlen(payload));

    // รวม CRC (uppercase hex, 4 หลัก)
    sprintf(output, "%s%04X", payload, crc);
}

int main() {
    char qr[1024];

    // ตัวอย่าง: เบอร์โทร + จำนวนเงิน
    const char *phone = "0944574687";
    const char *amount = "40.00";

    build_promptpay_qr(phone, amount, qr);

    printf("QR String: %s\n", qr);

    return 0;
}
