#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

// คำนวณ CRC16-CCITT (poly=0x1021, init=0xFFFF) พร้อม debug
uint16_t crc16_ccitt_debug(const char *data, size_t length) {
    uint16_t crc = 0xFFFF;
    printf("Initial CRC = 0x%04X\n", crc);

    for (size_t i = 0; i < length; i++) {
        uint8_t byte = (uint8_t)data[i];
        crc ^= (uint16_t)byte << 8;
        printf("\nByte %zu = 0x%02X, CRC after XOR = 0x%04X\n", i, byte, crc);

        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
                printf("  Bit %d: MSB=1 -> shift+XOR, CRC=0x%04X\n", j, crc);
            } else {
                crc <<= 1;
                printf("  Bit %d: MSB=0 -> shift only , CRC=0x%04X\n", j, crc);
            }
        }
    }

    printf("\nFinal CRC = 0x%04X\n", crc & 0xFFFF);
    return crc & 0xFFFF;
}

int main() {
    const char *test = "123456789";  // ข้อมูลมาตรฐานทดสอบ CRC
    uint16_t result = crc16_ccitt_debug(test, 9);
    printf("\nResult CRC16-CCITT = 0x%04X\n", result);
    return 0;
}
