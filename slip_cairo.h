#ifndef SLIP_CAIRO_H
#define SLIP_CAIRO_H

#include <stddef.h>
#include <stdint.h>
#include "listbox.h"
#include <fcntl.h>     // สำหรับ open(), O_WRONLY, O_CREAT ฯลฯ
#include <unistd.h>    // สำหรับ write(), close()


// ------------ ฟังก์ชัน ---------------------------
void print_slip_to_printer(AppWidgets *app, const char *filename);
void print_slip_full_cairo(AppWidgets *app, Order *order);

// ฟังก์ชันคำนวณ CRC16-CCITT
uint16_t crc16_ccitt(const char *data, size_t length);

// สร้างข้อมูลสำหรับ QR PromptPay
char* build_promptpay_qr(const char *phone, const char *amount);

#endif // SLIP_CAIRO_H
