//#include "slip.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <cairo.h>
//#include <pango/pangocairo.h>
//#include <qrencode.h>
//#include <ctype.h>
//#include <stdint.h>

//// คำนวณ CRC16-CCITT (poly=0x1021, init=0xFFFF)
//uint16_t crc16_ccitt(const char *data, size_t length) {
    //uint16_t crc = 0xFFFF;
    //for (size_t i = 0; i < length; i++) {
        //crc ^= (uint8_t)data[i] << 8;
        //for (int j = 0; j < 8; j++) {
            //if (crc & 0x8000)
                //crc = (crc << 1) ^ 0x1021;
            //else
                //crc <<= 1;
        //}
    //}
    //return crc & 0xFFFF;
//}

//// สร้าง PromptPay QR พร้อม Ref1 (order_id)
//char* build_promptpay_qr(const char *phone, const char *amount, int order_id) {
    //char merchantInfo[128];
    //char addData[32];
    //char payload[512];


//char order_id_str[16];   // แปลงเลข order_id เป็น string

    //// แปลงเบอร์เป็น 0066XXXXXXXXX
    //char phoneProxy[32];
    //if (phone[0] == '0') {
        //sprintf(phoneProxy, "0066%s", phone + 1);
    //} else {
        //sprintf(phoneProxy, "0066%s", phone);
    //}

    //// Merchant Account Information (ID 29)
    //sprintf(merchantInfo,
        //"0016A000000677010111"
        //"0113%s", phoneProxy
    //);

    //sprintf(order_id_str, "%d", order_id);   // order_id เป็น int
    //// สร้าง Ref1
    //sprintf(addData, "05%02zu%s", strlen(order_id_str), order_id_str);
    
    //// สร้าง payload (ยังไม่รวม CRC)
    //sprintf(payload,
        //"000201"              // Payload Format Indicator
        //"010212"              // Point of Initiation Method (12 = dynamic)
        //"29%02zu%s"           // Merchant Account Info
        //"5802TH"              // Country Code
        //"5303764"             // Currency THB
        //"54%02zu%s"           // Amount
        //"62%02zu%s"           // Additional Data Template
        //"6304",               // CRC placeholder
        //strlen(merchantInfo), merchantInfo,
        //strlen(amount), amount,
        //strlen(addData), addData
    //);

    //uint16_t crc = crc16_ccitt(payload, strlen(payload));

    //char *output = (char*)malloc(strlen(payload) + 5);
    //if (!output) return NULL;

    //sprintf(output, "%s%04X", payload, crc);
    //return output;
//}

//// ---------------- Draw Text -----------------
//void draw_text(PangoLayout *layout, cairo_t *cr, int x, int *y, const char *text, int font_size, int bold) {
    //pango_layout_set_text(layout, text, -1);
    //char font_desc[64];
    //snprintf(font_desc, sizeof(font_desc), "%s %d", bold ? "Noto Sans Thai Bold" : "Noto Sans Thai", font_size);
    //PangoFontDescription *desc = pango_font_description_from_string(font_desc);
    //pango_layout_set_font_description(layout, desc);
    //pango_font_description_free(desc);

    //cairo_set_source_rgb(cr, 0, 0, 0);
    //cairo_move_to(cr, x, *y);
    //pango_cairo_show_layout(cr, layout);

    //int height;
    //pango_layout_get_pixel_size(layout, NULL, &height);
    //*y += height + 5;
//}

//void draw_center_text(PangoLayout *layout, cairo_t *cr, int surface_width, int *y,
                      //const char *text, int font_size, int bold) {
    //pango_layout_set_text(layout, text, -1);
    //char font_desc[64];
    //snprintf(font_desc, sizeof(font_desc), "%s %d",
             //bold ? "Noto Sans Thai Bold" : "Noto Sans Thai", font_size);
    //PangoFontDescription *desc = pango_font_description_from_string(font_desc);
    //pango_layout_set_font_description(layout, desc);
    //pango_font_description_free(desc);

    //int width, height;
    //pango_layout_get_pixel_size(layout, &width, &height);
    //int x = (surface_width - width) / 2;

    //cairo_set_source_rgb(cr, 0, 0, 0);
    //cairo_move_to(cr, x, *y);
    //pango_cairo_show_layout(cr, layout);

    //*y += height + 5;
//}

//// ---------------- Print Slip -----------------
//void print_slip_full(Order *order) {
    //if (!order) return;

    //int surface_width = 400;
    //int y = 10;
    //char buf[256];

    //// วัดขนาด
    //cairo_surface_t *rec_surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, NULL);
    //cairo_t *rec_cr = cairo_create(rec_surface);
    //PangoLayout *layout = pango_cairo_create_layout(rec_cr);

    //snprintf(buf, sizeof(buf), "ออเดอร์ #%d", order->order_id);
    //draw_text(layout, rec_cr, 10, &y, buf, 18, 1);
    //snprintf(buf, sizeof(buf), "ลูกค้า: %s", order->line_name);
    //draw_text(layout, rec_cr, 10, &y, buf, 16, 0);
    //snprintf(buf, sizeof(buf), "สถานที่ส่ง: %s", order->place);
    //draw_text(layout, rec_cr, 10, &y, buf, 16, 0);
    //snprintf(buf, sizeof(buf), "เวลาส่ง: %s", order->delivery_time);
    //draw_text(layout, rec_cr, 10, &y, buf, 16, 0);
    //snprintf(buf, sizeof(buf), "สถานะ: %s", order->statusText);
    //draw_text(layout, rec_cr, 10, &y, buf, 16, 1);
    //snprintf(buf, sizeof(buf), "วันที่สั่ง: %s", order->created_at);
    //draw_text(layout, rec_cr, 10, &y, buf, 16, 0);

    //draw_text(layout, rec_cr, 10, &y, "รายการ:", 16, 1);
    //double totalAmount = 0;

    //for (int i = 0; i < order->item_count; i++) {
        //snprintf(buf, sizeof(buf), "- %s %d x %.2f", 
                 //order->items[i].name, order->items[i].qty, order->items[i].price);
        //draw_text(layout, rec_cr, 20, &y, buf, 16, 0);
        //totalAmount += order->items[i].price * order->items[i].qty;

        //if (strlen(order->items[i].option_text) > 0) {
            //snprintf(buf, sizeof(buf), "(%s)", order->items[i].option_text);
            //draw_text(layout, rec_cr, 40, &y, buf, 12, 0);
        //}
    //}

    //snprintf(buf, sizeof(buf), "รวมทั้งหมด: %.2f บาท", totalAmount);
    //draw_text(layout, rec_cr, 10, &y, buf, 16, 1);

    //cairo_destroy(rec_cr);
    //cairo_surface_destroy(rec_surface);
    //g_object_unref(layout);

    //// Render จริง
    //cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, surface_width, y + 150);
    //cairo_t *cr = cairo_create(surface);
    //layout = pango_cairo_create_layout(cr);

    //cairo_set_source_rgb(cr, 1,1,1);
    //cairo_paint(cr);

    //y = 10;
    //snprintf(buf, sizeof(buf), "ออเดอร์ #%d", order->order_id);
    //draw_text(layout, cr, 10, &y, buf, 18, 1);
    //snprintf(buf, sizeof(buf), "ลูกค้า: %s", order->line_name);
    //draw_text(layout, cr, 10, &y, buf, 16, 0);
    //snprintf(buf, sizeof(buf), "สถานที่ส่ง: %s", order->place);
    //draw_text(layout, cr, 10, &y, buf, 16, 0);
    //snprintf(buf, sizeof(buf), "เวลาส่ง: %s", order->delivery_time);
    //draw_text(layout, cr, 10, &y, buf, 16, 0);
    //snprintf(buf, sizeof(buf), "วันที่สั่ง: %s", order->created_at);
    //draw_text(layout, cr, 10, &y, buf, 16, 0);

    //draw_text(layout, cr, 10, &y, "รายการ:", 16, 1);
    //totalAmount = 0;
    //for (int i = 0; i < order->item_count; i++) {
        //snprintf(buf, sizeof(buf), "- %s %d x %.2f", 
                 //order->items[i].name, order->items[i].qty, order->items[i].price);
        //draw_text(layout, cr, 20, &y, buf, 16, 0);
        //totalAmount += order->items[i].price * order->items[i].qty;

        //if (strlen(order->items[i].option_text) > 0) {
            //snprintf(buf, sizeof(buf), "(%s)", order->items[i].option_text);
            //draw_text(layout, cr, 40, &y, buf, 12, 0);
        //}
    //}
    //snprintf(buf, sizeof(buf), "รวมทั้งหมด: %.2f บาท", totalAmount);
    //draw_text(layout, cr, 10, &y, buf, 16, 1);

    //// -------- QR PromptPay (กลางล่าง) --------
    //char amount_str[16]="";
    //if(totalAmount>0) snprintf(amount_str,sizeof(amount_str),"%.2f",totalAmount);

    //char *qrdata = build_promptpay_qr("0944574687", amount_str, order->order_id);
    //if (!qrdata) {
        //fprintf(stderr, "Failed to build QR data\n");
        //return;
    //}

    //y += 20;

    //QRcode *qrcode = QRcode_encodeString(qrdata, 0, QR_ECLEVEL_Q, QR_MODE_8, 1);
    //free(qrdata);

    //int scale = 4;
    //int qr_margin = 4;

    //if (qrcode) {
        //int qr_draw_size = (qrcode->width + 2*qr_margin) * scale;
        //int qr_x = (surface_width - qr_draw_size)/2;
        //int qr_y = y;

        //cairo_set_source_rgb(cr, 1,1,1);
        //cairo_rectangle(cr, qr_x, qr_y, qr_draw_size, qr_draw_size);
        //cairo_fill(cr);

        //cairo_save(cr);
        //cairo_translate(cr, qr_x + qr_margin*scale, qr_y + qr_margin*scale);
        //cairo_set_source_rgb(cr, 0,0,0);
        //for (int yy = 0; yy < qrcode->width; yy++)
            //for (int xx = 0; xx < qrcode->width; xx++)
                //if (qrcode->data[yy * qrcode->width + xx] & 1)
                    //cairo_rectangle(cr, xx*scale, yy*scale, scale, scale);
        //cairo_fill(cr);
        //cairo_restore(cr);

        //QRcode_free(qrcode);
        //y = qr_y + qr_draw_size;
    //}

    //char buf2[128];
    //snprintf(buf2, sizeof(buf2), "%s", "0944574687");
    //draw_center_text(layout, cr, surface_width, &y, buf2, 14, 0);

    //y += 10;

    //snprintf(buf2, sizeof(buf2), "%s", "บัญชี: โยธิน อินบรรเลง");
    //draw_center_text(layout, cr, surface_width, &y, buf2, 14, 0);

    //// -------- เขียน PNG --------
    //cairo_surface_write_to_png(surface, "/tmp/slip.png");

    //cairo_destroy(cr);
    //cairo_surface_destroy(surface);
    //g_object_unref(layout);

    //printf("✅ สลิปสร้างเสร็จแล้ว: /tmp/slip.png\n");
//}


#include "slip.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <qrencode.h>
#include <ctype.h>
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

char* build_promptpay_qr(const char *phone, const char *amount) {
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
    
    // จอง memory สำหรับ return
    char *output = (char*)malloc(strlen(payload) + 5); // +4 for CRC + 1 for null
    if (!output) return NULL;

    // รวม CRC (uppercase hex, 4 หลัก)
    sprintf(output, "%s%04X", payload, crc);
    
    return output;
}

// ---------------- Draw Text -----------------
void draw_text(PangoLayout *layout, cairo_t *cr, int x, int *y, const char *text, int font_size, int bold) {
    pango_layout_set_text(layout, text, -1);
    char font_desc[64];
    snprintf(font_desc, sizeof(font_desc), "%s %d", bold ? "Noto Sans Thai Bold" : "Noto Sans Thai", font_size);
    PangoFontDescription *desc = pango_font_description_from_string(font_desc);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);
    
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, x, *y);
    pango_cairo_show_layout(cr, layout);

    int height;
    pango_layout_get_pixel_size(layout, NULL, &height);
    *y += height + 5;
}

// วาดข้อความแบบจัดกึ่งกลาง
void draw_center_text(PangoLayout *layout, cairo_t *cr, int surface_width, int *y,
                      const char *text, int font_size, int bold) {
    pango_layout_set_text(layout, text, -1);
    char font_desc[64];
    snprintf(font_desc, sizeof(font_desc), "%s %d",
             bold ? "Noto Sans Thai Bold" : "Noto Sans Thai", font_size);
    PangoFontDescription *desc = pango_font_description_from_string(font_desc);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    int width, height;
    pango_layout_get_pixel_size(layout, &width, &height);
    int x = (surface_width - width) / 2;

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, x, *y);
    pango_cairo_show_layout(cr, layout);

    *y += height + 5;
}

// ---------------- Print Slip -----------------
void print_slip_full(Order *order) {
    if (!order) return;

    int surface_width = 400;
    int y = 10;
    char buf[256];

    // วัดขนาด
    cairo_surface_t *rec_surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, NULL);
    cairo_t *rec_cr = cairo_create(rec_surface);
    PangoLayout *layout = pango_cairo_create_layout(rec_cr);

    snprintf(buf, sizeof(buf), "ออเดอร์ #%d", order->order_id);
    draw_text(layout, rec_cr, 10, &y, buf, 18, 1);
    snprintf(buf, sizeof(buf), "ลูกค้า: %s", order->line_name);
    draw_text(layout, rec_cr, 10, &y, buf, 16, 0);
    snprintf(buf, sizeof(buf), "สถานที่ส่ง: %s", order->place);
    draw_text(layout, rec_cr, 10, &y, buf, 16, 0);
    snprintf(buf, sizeof(buf), "เวลาส่ง: %s", order->delivery_time);
    draw_text(layout, rec_cr, 10, &y, buf, 16, 0);
    snprintf(buf, sizeof(buf), "สถานะ: %s", order->statusText);
    draw_text(layout, rec_cr, 10, &y, buf, 16, 1);
    snprintf(buf, sizeof(buf), "วันที่สั่ง: %s", order->created_at);
    draw_text(layout, rec_cr, 10, &y, buf, 16, 0);

    draw_text(layout, rec_cr, 10, &y, "รายการ:", 16, 1);
    double totalAmount = 0;
    
    for (int i = 0; i < order->item_count; i++) {
      // แสดงชื่อสินค้า
      snprintf(buf, sizeof(buf), "- %s %d x %.2f", 
               order->items[i].name, 
               order->items[i].qty, 
               order->items[i].price);
      draw_text(layout, rec_cr, 20, &y, buf, 16, 0);

      totalAmount += order->items[i].price * order->items[i].qty;
      // ถ้ามี option_text ให้แสดงด้านล่าง
      if (strlen(order->items[i].option_text) > 0) {
          snprintf(buf, sizeof(buf), "(%s)", order->items[i].option_text);
          draw_text(layout, rec_cr, 40, &y, buf, 12, 0);  
          // ↑ ใช้ x=40 เพื่อเว้นจากข้างหน้าเล็กน้อย
      }
    }
    
    snprintf(buf, sizeof(buf), "รวมทั้งหมด: %.2f บาท", totalAmount);
    draw_text(layout, rec_cr, 10, &y, buf, 16, 1);

    int qr_size = 120;
    y += qr_size + 20;
    int total_height = y;

    cairo_destroy(rec_cr);
    cairo_surface_destroy(rec_surface);
    g_object_unref(layout);

    // Render จริง
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, surface_width, total_height + 150);
    cairo_t *cr = cairo_create(surface);
    layout = pango_cairo_create_layout(cr);

    cairo_set_source_rgb(cr, 1,1,1);
    cairo_paint(cr);

    y = 10;
    snprintf(buf, sizeof(buf), "ออเดอร์ #%d", order->order_id);
    draw_text(layout, cr, 10, &y, buf, 18, 1);
    snprintf(buf, sizeof(buf), "ลูกค้า: %s", order->line_name);
    draw_text(layout, cr, 10, &y, buf, 16, 0);
    snprintf(buf, sizeof(buf), "สถานที่ส่ง: %s", order->place);
    draw_text(layout, cr, 10, &y, buf, 16, 0);
    snprintf(buf, sizeof(buf), "เวลาส่ง: %s", order->delivery_time);
    draw_text(layout, cr, 10, &y, buf, 16, 0);
    snprintf(buf, sizeof(buf), "วันที่สั่ง: %s", order->created_at);
    draw_text(layout, cr, 10, &y, buf, 16, 0);

    draw_text(layout, cr, 10, &y, "รายการ:", 16, 1);
    totalAmount = 0;

    for (int i = 0; i < order->item_count; i++) {
        snprintf(buf, sizeof(buf), "- %s %d x %.2f", order->items[i].name, order->items[i].qty, order->items[i].price);
        draw_text(layout, cr, 20, &y, buf, 16, 0);
        totalAmount += order->items[i].price * order->items[i].qty;
            // แสดง option_text ถ้ามี
        if (strlen(order->items[i].option_text) > 0) {
            snprintf(buf, sizeof(buf), "(%s)", order->items[i].option_text);
            draw_text(layout, cr, 40, &y, buf, 12, 0);
            //g_print("Option (cr): %s\n", order->items[i].option_text);
        }
    }
    snprintf(buf, sizeof(buf), "รวมทั้งหมด: %.2f บาท", totalAmount);
    draw_text(layout, cr, 10, &y, buf, 16, 1);

    // -------- QR PromptPay (กลางล่าง) --------
    char amount_str[16]="";
    if(totalAmount>0) snprintf(amount_str,sizeof(amount_str),"%.2f",totalAmount);
    
    char *qrdata = build_promptpay_qr("0944574687", amount_str);
    if (!qrdata) {
      fprintf(stderr, "Failed to build QR data\n");
      return;
    }
    
    y += 20;  // เลื่อนลง 1 บรรทัด

    QRcode *qrcode = QRcode_encodeString(qrdata, 0, QR_ECLEVEL_Q, QR_MODE_8, 1);
    free(qrdata);
    
int scale = 4;
int qr_margin = 4;

cairo_surface_t *logo = cairo_image_surface_create_from_png("PromptPay-logo.png");
if (logo) {
    int qr_draw_size = (qrcode->width + 2*qr_margin) * scale;
    int logo_width = qr_draw_size;

    // คำนวณความสูงตามอัตราส่วนจริง
    int logo_height = (int)((double)logo_width / cairo_image_surface_get_width(logo) 
                                         * cairo_image_surface_get_height(logo));

    // จำกัดขนาดโลโก้ไม่ให้เกิน 60% ของ QR
    int max_logo_width = (int)(qr_draw_size * 0.9);
    if (logo_width > max_logo_width) {
        logo_width = max_logo_width;
        logo_height = (int)((double)logo_width / cairo_image_surface_get_width(logo) 
                                         * cairo_image_surface_get_height(logo));
    }

    int logo_x = (surface_width - logo_width) / 2;
    int logo_y = y;

    cairo_save(cr);
    cairo_translate(cr, logo_x, logo_y);
    cairo_scale(cr, (double)logo_width / cairo_image_surface_get_width(logo),
                     (double)logo_height / cairo_image_surface_get_height(logo));
    cairo_set_source_surface(cr, logo, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);

    cairo_surface_destroy(logo);

    // เว้นระยะเล็กน้อยให้ QR ไม่ชนโลโก้
//    int spacing = 8;  
    y = logo_y + logo_height;
}


    if (qrcode) {
        int scale = 4;
        int qr_margin = 4;
        int qr_draw_size = (qrcode->width + 2*qr_margin) * scale;
        int qr_x = (surface_width - qr_draw_size)/2;
        int qr_y = y;

        cairo_set_source_rgb(cr, 1,1,1);
        cairo_rectangle(cr, qr_x, qr_y, qr_draw_size, qr_draw_size);
        cairo_fill(cr);

        cairo_save(cr);
        cairo_translate(cr, qr_x + qr_margin*scale, qr_y + qr_margin*scale);
        cairo_set_source_rgb(cr, 0,0,0);
        for (int yy = 0; yy < qrcode->width; yy++)
            for (int xx = 0; xx < qrcode->width; xx++)
                if (qrcode->data[yy * qrcode->width + xx] & 1)
                    cairo_rectangle(cr, xx*scale, yy*scale, scale, scale);
        cairo_fill(cr);
        cairo_restore(cr);

        QRcode_free(qrcode);
        
        y = qr_y + qr_draw_size; // เลื่อน y ลงสำหรับเบอร์โทรและจำนวนเงิน
    }
    
    char buf2[128];
    snprintf(buf2, sizeof(buf2), "%s", "0944574687");
    draw_center_text(layout, cr, surface_width, &y, buf2, 14, 0);

    y += 10;

    snprintf(buf2, sizeof(buf2), "%s", "บัญชี: โยธิน อินบรรเลง");
    draw_center_text(layout, cr, surface_width, &y, buf2, 14, 0);

    // -------- QR Order ID (มุมบนขวา) --------
    char order_qr[32];
    snprintf(order_qr, sizeof(order_qr), "%d", order->order_id);
    QRcode *order_qrcode = QRcode_encodeString(order_qr, 0, QR_ECLEVEL_Q, QR_MODE_8, 1);

    if (order_qrcode) {
        int scale = 2;
        int margin = 2;
        int qr_draw_size = (order_qrcode->width + 2*margin) * scale;
        int qr_x = surface_width - qr_draw_size - 10;
        int qr_y = 10;

        cairo_set_source_rgb(cr, 1,1,1);
        cairo_rectangle(cr, qr_x, qr_y, qr_draw_size, qr_draw_size);
        cairo_fill(cr);

        cairo_save(cr);
        cairo_translate(cr, qr_x + margin*scale, qr_y + margin*scale);
        cairo_set_source_rgb(cr, 0,0,0);
        for (int yy = 0; yy < order_qrcode->width; yy++)
            for (int xx = 0; xx < order_qrcode->width; xx++)
                if (order_qrcode->data[yy * order_qrcode->width + xx] & 1)
                    cairo_rectangle(cr, xx*scale, yy*scale, scale, scale);
        cairo_fill(cr);
        cairo_restore(cr);

        QRcode_free(order_qrcode);
    }



    // -------- เขียน PNG --------
    cairo_surface_write_to_png(surface, "/tmp/slip.png");

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(layout);

    printf("✅ สลิปสร้างเสร็จแล้ว: /tmp/slip.png\n");
}
