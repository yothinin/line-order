#include "slip_cairo.h"
#include "listbox.h"
#include "qrpayment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <qrencode.h>
#include <ctype.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#define PRINTER_DEVICE "/dev/usb/lp1"

void print_bmp_escpos_chunked(const char *bmp_path) {
    FILE *bmp = fopen(bmp_path, "rb");
    if (!bmp) {
        perror("❌ เปิดไฟล์ BMP ไม่ได้");
        return;
    }

    unsigned char header[54];
    if (fread(header, 1, 54, bmp) != 54) {
        fprintf(stderr, "❌ อ่าน header BMP ไม่ครบ\n");
        fclose(bmp);
        return;
    }

    // Debug: แสดง header ทั้งหมด
    printf("🔍 BMP Header: ");
    for (int i = 0; i < 54; i++) {
        printf("%02X ", header[i]);
    }
    printf("\n");

    // อ่าน width/height แบบ little-endian
    int width  = header[18] | (header[19] << 8) | (header[20] << 16) | (header[21] << 24);
    int height = header[22] | (header[23] << 8) | (header[24] << 16) | (header[25] << 24);

    printf("📏 width=%d height=%d\n", width, height);

    int row_padded = (width * 3 + 3) & (~3);
    printf("📏 row_padded=%d\n", row_padded);

    unsigned char *row = malloc(row_padded);
    if (!row) {
        fprintf(stderr, "❌ ไม่สามารถจัดสรรหน่วยความจำ\n");
        fclose(bmp);
        return;
    }

    int fd = open(PRINTER_DEVICE, O_WRONLY);
    if (fd < 0) {
        perror("❌ เปิดเครื่องพิมพ์ไม่ได้");
        free(row);
        fclose(bmp);
        return;
    }

    printf("📤 ส่งคำสั่ง Initialize Printer\n");
    unsigned char init[] = {0x1B, 0x40};
    write(fd, init, sizeof(init));

    printf("📤 ส่งคำสั่ง Set line spacing\n");
    unsigned char line_spacing[] = {0x1B, 0x33, 24};
    write(fd, line_spacing, sizeof(line_spacing));

    int bytes_per_line = (width + 7) / 8;
    unsigned char header_raster[] = {
        0x1D, 0x76, 0x30, 0x00,
        bytes_per_line & 0xFF, (bytes_per_line >> 8) & 0xFF,
        height & 0xFF, (height >> 8) & 0xFF
    };
    write(fd, header_raster, sizeof(header_raster));

    printf("📤 เริ่มส่งภาพทีละบรรทัดแบบ chunked\n");

    for (int y = height - 1; y >= 0; y--) {
        if (fseek(bmp, 54 + y * row_padded, SEEK_SET) != 0) {
            perror("❌ fseek ล้มเหลว");
            break;
        }

        if (fread(row, 1, row_padded, bmp) != row_padded) {
            fprintf(stderr, "❌ อ่าน row ล้มเหลวที่ y=%d\n", y);
            break;
        }

        for (int x_byte = 0; x_byte < bytes_per_line; x_byte++) {
            unsigned char byte = 0;
            for (int bit = 0; bit < 8; bit++) {
                int x = x_byte * 8 + bit;
                if (x >= width) continue;

                int pixel_index = x * 3;
                unsigned char b = row[pixel_index];
                unsigned char g = row[pixel_index + 1];
                unsigned char r = row[pixel_index + 2];

                int gray = (r * 299 + g * 587 + b * 114) / 1000;
                if (gray < 128) {
                    byte |= (1 << (7 - bit));
                }
            }

            ssize_t written = write(fd, &byte, 1);
            if (written != 1) {
                perror("❌ เขียน byte ล้มเหลว");
                printf("📌 debug: y=%d, x_byte=%d, byte=0x%02X\n", y, x_byte, byte);
                goto end;
            }
        }
    }

    printf("📤 ส่งคำสั่ง Feed paper\n");
    unsigned char feed[] = {0x1B, 0x64, 5};
    if (write(fd, feed, sizeof(feed)) != sizeof(feed)) {
        perror("❌ เขียน feed command ล้มเหลว");
    }

end:
    close(fd);
    free(row);
    fclose(bmp);

    printf("✅ พิมพ์ %s เรียบร้อย\n", bmp_path);
}

//void print_bmp_escpos(const char *bmp_path) {
    //FILE *bmp = fopen(bmp_path, "rb");
    //if (!bmp) {
        //perror("❌ เปิดไฟล์ BMP ไม่ได้");
        //return;
    //}

    //// อ่าน header BMP
    //unsigned char header[54];
    //fread(header, 1, 54, bmp);

    //int width  = *(int*)&header[18];
    //int height = *(int*)&header[22];
    //int row_padded = (width*3 + 3) & (~3);

    //unsigned char *row = malloc(row_padded);
    //if (!row) {
        //fprintf(stderr, "❌ ไม่สามารถจัดสรรหน่วยความจำ\n");
        //fclose(bmp);
        //return;
    //}

    //int fd = open(PRINTER_DEVICE, O_WRONLY);
    //if (fd < 0) {
        //perror("❌ เปิดเครื่องพิมพ์ไม่ได้");
        //free(row);
        //fclose(bmp);
        //return;
    //}

    //// ESC/POS: Initialize printer
    //unsigned char init[] = {0x1B, 0x40};
    //write(fd, init, sizeof(init));

    //// ESC/POS: Set line spacing to 24 dots
    //unsigned char line_spacing[] = {0x1B, 0x33, 24};
    //write(fd, line_spacing, sizeof(line_spacing));

    //int bytes_per_line = (width + 7) / 8;
    //unsigned char header_raster[] = {
        //0x1D, 0x76, 0x30, 0x00,
        //bytes_per_line & 0xFF, (bytes_per_line >> 8) & 0xFF,
        //height & 0xFF, (height >> 8) & 0xFF
    //};
    //write(fd, header_raster, sizeof(header_raster));

    //for (int y = height - 1; y >= 0; y--) {
        //fseek(bmp, 54 + y*row_padded, SEEK_SET);
        //fread(row, 1, row_padded, bmp);

        //for (int x_byte = 0; x_byte < bytes_per_line; x_byte++) {
            //unsigned char byte = 0;
            //for (int bit = 0; bit < 8; bit++) {
                //int x = x_byte*8 + bit;
                //if (x >= width) continue;

                //int pixel_index = x*3;
                //unsigned char b = row[pixel_index];
                //unsigned char g = row[pixel_index+1];
                //unsigned char r = row[pixel_index+2];

                //int gray = (r*299 + g*587 + b*114) / 1000;
                //if (gray < 128) {
                    //byte |= (1 << (7-bit));
                //}
            //}
            //write(fd, &byte, 1);
        //}
    //}

    //// Feed paper
    //unsigned char feed[] = {0x1B, 0x64, 5};
    //write(fd, feed, sizeof(feed));

    //close(fd);
    //free(row);
    //fclose(bmp);

    //printf("✅ พิมพ์ %s เรียบร้อย\n", bmp_path);
//}

void print_slip_to_printer_via_cups(const char *filename) {
    // โหลด PNG แล้วแปลงเป็น bitmap
    cairo_surface_t *image = cairo_image_surface_create_from_png(filename);
    if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "❌ ไม่สามารถโหลดไฟล์ PNG: %s\n", filename);
        return;
    }

    int width = cairo_image_surface_get_width(image);
    int height = cairo_image_surface_get_height(image);

    unsigned char *data = cairo_image_surface_get_data(image);
    int stride = cairo_image_surface_get_stride(image);

    unsigned char *bitmap = malloc(width * height);
    if (!bitmap) {
        fprintf(stderr, "❌ ไม่สามารถจัดสรรหน่วยความจำได้\n");
        cairo_surface_destroy(image);
        return;
    }

    for (int y = 0; y < height; y++) {
        unsigned char *row_data = data + y * stride;
        for (int x = 0; x < width; x++) {
            unsigned char *pixel = row_data + x * 4;
            int r = pixel[2];
            int g = pixel[1];
            int b = pixel[0];
            int gray = (r*299 + g*587 + b*114) / 1000;
            bitmap[y * width + x] = (gray < 128) ? 0 : 1;
        }
    }

    // สร้างไฟล์ BMP
    const char *bmp_path = "/tmp/slip.bmp";
    FILE *bmp = fopen(bmp_path, "wb");
    if (!bmp) {
        perror("❌ เปิดไฟล์ slip.bmp ไม่ได้");
        free(bitmap);
        cairo_surface_destroy(image);
        return;
    }

    unsigned char header[54] = {0};
    int file_size = 54 + width * height * 3;
    header[0] = 'B'; header[1] = 'M';
    header[2] = file_size & 0xFF;
    header[3] = (file_size >> 8) & 0xFF;
    header[4] = (file_size >> 16) & 0xFF;
    header[5] = (file_size >> 24) & 0xFF;
    header[10] = 54;
    header[14] = 40;
    header[18] = width & 0xFF;
    header[19] = (width >> 8) & 0xFF;
    header[20] = (width >> 16) & 0xFF;
    header[21] = (width >> 24) & 0xFF;
    header[22] = height & 0xFF;
    header[23] = (height >> 8) & 0xFF;
    header[24] = (height >> 16) & 0xFF;
    header[25] = (height >> 24) & 0xFF;
    header[26] = 1;
    header[28] = 24;

    fwrite(header, 1, 54, bmp);

    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            unsigned char color = bitmap[y * width + x] ? 255 : 0;
            unsigned char pixel[3] = {color, color, color};
            fwrite(pixel, 1, 3, bmp);
        }
        int padding = (4 - (width * 3) % 4) % 4;
        for (int p = 0; p < padding; p++) fputc(0, bmp);
    }
    fclose(bmp);

    printf("✅ สร้างไฟล์ BMP: %s\n", bmp_path);

    free(bitmap);
    cairo_surface_destroy(image);

    //print_bmp_escpos_chunked("/tmp/slip.bmp");               // ส่งไปพิมพ์ ESC/POS

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
void print_slip_full_cairo(AppWidgets *app, Order *order) {
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
        snprintf(buf, sizeof(buf), "- %s %d x %.2f", order->items[i].name, order->items[i].qty, order->items[i].price);
        draw_text(layout, rec_cr, 20, &y, buf, 16, 0);
        totalAmount += order->items[i].price * order->items[i].qty;
        if (strlen(order->items[i].option_text) > 0) {
            snprintf(buf, sizeof(buf), "(%s)", order->items[i].option_text);
            draw_text(layout, rec_cr, 40, &y, buf, 12, 0);
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
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, surface_width, total_height + 200);
    cairo_t *cr = cairo_create(surface);
    layout = pango_cairo_create_layout(cr);

    cairo_set_source_rgb(cr, 1, 1, 1);
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
        if (strlen(order->items[i].option_text) > 0) {
            snprintf(buf, sizeof(buf), "(%s)", order->items[i].option_text);
            draw_text(layout, cr, 40, &y, buf, 12, 0);
        }
    }

    snprintf(buf, sizeof(buf), "รวมทั้งหมด: %.2f บาท", totalAmount);
    draw_text(layout, cr, 10, &y, buf, 16, 1);

    // -------- QR PromptPay --------
    char amount_str[16] = "";
    if (totalAmount > 0) snprintf(amount_str, sizeof(amount_str), "%.2f", totalAmount);

    char *qrdata = build_promptpay_qr("0944574687", amount_str);
    if (!qrdata) {
        fprintf(stderr, "Failed to build QR data\n");
        return;
    }

    y += 20;

    QRcode *qrcode = QRcode_encodeString(qrdata, 0, QR_ECLEVEL_Q, QR_MODE_8, 1);
    free(qrdata);

    if (qrcode) {
        int scale = 5;
        int qr_margin = 4;
        int qr_draw_size = (qrcode->width + 2 * qr_margin) * scale;
        int qr_x = (surface_width - qr_draw_size) / 2;
        int qr_y = y;

        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_rectangle(cr, qr_x, qr_y, qr_draw_size, qr_draw_size);
        cairo_fill(cr);

        cairo_save(cr);
        cairo_translate(cr, qr_x + qr_margin * scale, qr_y + qr_margin * scale);
        cairo_set_source_rgb(cr, 0, 0, 0);
        for (int yy = 0; yy < qrcode->width; yy++)
            for (int xx = 0; xx < qrcode->width; xx++)
                if (qrcode->data[yy * qrcode->width + xx] & 1)
                    cairo_rectangle(cr, xx * scale, yy * scale, scale, scale);
        cairo_fill(cr);
        cairo_restore(cr);

        QRcode_free(qrcode);

        y = qr_y + qr_draw_size + 10;

        // -------- โลโก้ PromptPay --------
        cairo_surface_t *logo = cairo_image_surface_create_from_png("PromptPay-logo.png");
        if (logo) {
            int logo_width = qr_draw_size;
            int logo_height = (int)((double)logo_width / cairo_image_surface_get_width(logo) 
                                                * cairo_image_surface_get_height(logo));

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

            y += logo_height + 10;
        }

        // -------- เบอร์ PromptPay --------
        g_object_unref(layout);
        layout = pango_cairo_create_layout(cr);
        cairo_set_source_rgb(cr, 0, 0, 0);
        snprintf(buf, sizeof(buf), "%s", "0944574687");
        draw_center_text(layout, cr, surface_width, &y, buf, 18, 0);

        y += 20;

        // -------- ชื่อบัญชี --------
        snprintf(buf, sizeof(buf), "%s", "บัญชี: โยธิน อินบรรเลง");
        draw_center_text(layout, cr, surface_width, &y, buf, 14, 0);
    }

    //// -------- QR Order ID --------
    //char order_qr[32];
    //snprintf(order_qr, sizeof(order_qr), "%d", order->order_id);
    //QRcode *order_qrcode = QRcode_encodeString(order_qr, 0, QR_ECLEVEL_Q, QR_MODE_8, 1);

    //if (order_qrcode) {
        //int scale = 4;
        //int margin = 2;
        //int qr_draw_size = (order_qrcode->width + 2 * margin) * scale;
        //int qr_x = surface_width - qr_draw_size - 10;
        //int qr_y = 10;

        //cairo_set_source_rgb(cr, 1, 1, 1);
        //cairo_rectangle(cr, qr_x, qr_y, qr_draw_size, qr_draw_size);
        //cairo_fill(cr);

        //cairo_save(cr);
        //cairo_translate(cr, qr_x + margin * scale, qr_y + margin * scale);
        //cairo_set_source_rgb(cr, 0, 0, 0);
        //for (int yy = 0; yy < order_qrcode->width; yy++)
            //for (int xx = 0; xx < order_qrcode->width; xx++)
                //if (order_qrcode->data[yy * order_qrcode->width + xx] & 1)
                    //cairo_rectangle(cr, xx * scale, yy * scale, scale, scale);
        //cairo_fill(cr);
        //cairo_restore(cr);

        //QRcode_free(order_qrcode);
    //}

    // -------- เขียน PNG --------
    cairo_surface_write_to_png(surface, "/tmp/slip.png");
    cairo_surface_flush(surface);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(layout);

    printf("✅ สลิปสร้างเสร็จแล้ว: /tmp/slip.png\n");

    //print_bmp_escpos_chunked("/tmp/slip.png");
}
