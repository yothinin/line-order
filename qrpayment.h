#ifndef QRPAYMENT_H
#define QRPAYMENT_H

#include <stddef.h>  // สำหรับ size_t
#include <stddef.h>   // สำหรับ size_t
#include <stdint.h>   // สำหรับ uint16_t, uint8_t

uint16_t crc16_ccitt(const char *data, size_t length);
char* build_promptpay_qr(const char *phone, const char *amount);

#endif
