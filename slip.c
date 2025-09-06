#include "slip.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <qrencode.h>
#include <ctype.h>

// ---------------- CRC16 Table -----------------
static const unsigned short crcTable[256] = {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

// CRC16 แบบเดียวกับ JS
unsigned short crc16_js(const char *s) {
    unsigned short crc = 0xFFFF;
    for(size_t i=0;i<strlen(s);i++){
        unsigned char c = (unsigned char)s[i];
        unsigned char j = (c ^ (crc >> 8)) & 0xFF;
        crc = crcTable[j] ^ (crc << 8);
    }
    return crc & 0xFFFF;
}

// ---------------- Helper Functions -----------------
void sanitizeTarget(char *out, const char *in) {
    size_t j = 0;
    for (size_t i = 0; i < strlen(in); i++)
        if (isdigit(in[i])) out[j++] = in[i];
    out[j] = 0;
}

void formatTarget(char *out, const char *in) {
    char tmp[32];
    sanitizeTarget(tmp, in);
    size_t len = strlen(tmp);
    if (len >= 13) strcpy(out, tmp);
    else {
        char buf[16];
        snprintf(buf, sizeof(buf), "0000000000000%s", tmp+((tmp[0]=='0')?1:0));
        strcpy(out, buf + strlen(buf)-13);
    }
}

void formatAmount(char *out, double amount) {
    snprintf(out, 16, "%.2f", amount);
}

void formatTargetPromptPay(char *out, const char *in) {
    char tmp[32];
    size_t j=0;
    for(size_t i=0;i<strlen(in);i++)
        if(isdigit(in[i])) tmp[j++]=in[i];
    tmp[j]=0;

    size_t len = strlen(tmp);

    if(len >= 15) {          // e-wallet ID
        strcpy(out, tmp);
    } else if(len >= 13) {   // Tax ID
        strcpy(out, tmp);
    } else {                 // Mobile
        if(tmp[0]=='0')
            snprintf(out,32,"66%s", tmp+1); // นำ 0 เปลี่ยนเป็น 66
        else
            snprintf(out,32,"%s", tmp);
    }
}


char *generate_promptpay_full(const char *target, double amount) {
    char formatted[32];
    formatTargetPromptPay(formatted, target);

    char amount_str[16]="";
    if(amount>0) snprintf(amount_str,sizeof(amount_str),"%.2f",amount);

    // สร้าง payload ตาม JS
    char payload[512];
    snprintf(payload,sizeof(payload),
        "00020101021229370016A000000677010111011300%s5802TH53037645406%s",
        formatted,
        amount>0?amount_str:""
    );

    char crc_input[520];
    snprintf(crc_input,sizeof(crc_input),"%s6304",payload);

    unsigned short crc = crc16_js(crc_input);
    char crc_str[5];
    snprintf(crc_str,sizeof(crc_str),"%04X",crc);

    char *final = malloc(strlen(payload)+5);
    sprintf(final,"%s6304%s",payload,crc_str);
    return final;
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
        snprintf(buf, sizeof(buf), "- %s x%d  %.2f", order->items[i].name, order->items[i].qty, order->items[i].price);
        draw_text(layout, rec_cr, 20, &y, buf, 16, 0);
        totalAmount += order->items[i].price * order->items[i].qty;
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
        snprintf(buf, sizeof(buf), "- %s x%d  %.2f", order->items[i].name, order->items[i].qty, order->items[i].price);
        draw_text(layout, cr, 20, &y, buf, 16, 0);
        totalAmount += order->items[i].price * order->items[i].qty;
    }
    snprintf(buf, sizeof(buf), "รวมทั้งหมด: %.2f บาท", totalAmount);
    draw_text(layout, cr, 10, &y, buf, 16, 1);

    // -------- QR PromptPay (กลางล่าง) --------
    char *qrdata = generate_promptpay_full("0944574687", totalAmount);
    QRcode *qrcode = QRcode_encodeString(qrdata, 0, QR_ECLEVEL_Q, QR_MODE_8, 1);
    free(qrdata);

    if (qrcode) {
        int scale = 4;
        int qr_margin = 4;
        int qr_draw_size = (qrcode->width + 2*qr_margin) * scale;
        int qr_x = (surface_width - qr_draw_size)/2;
        int qr_y = y + 10;

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
    }

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
