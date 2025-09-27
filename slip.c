#include "utf8_conv.h"
#include "slip.h"
#include "qrpayment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <qrencode.h>
#include <png.h>
#include <iconv.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>


#define PRINTER_DEVICE "/dev/usb/lp1"

char *utf8_to_tis620(const char *utf8) {
    iconv_t cd = iconv_open("TIS-620", "UTF-8");
    if (cd == (iconv_t)-1) return NULL;

    size_t inbytes = strlen(utf8);
    size_t outbytes = inbytes * 2;
    char *outbuf = malloc(outbytes + 1);
    if (!outbuf) {
        iconv_close(cd);
        return NULL;
    }

    char *inptr = (char *)utf8;
    char *outptr = outbuf;
    size_t inleft = inbytes;
    size_t outleft = outbytes;

    memset(outbuf, 0, outbytes + 1);

    if (iconv(cd, &inptr, &inleft, &outptr, &outleft) == (size_t)-1) {
        free(outbuf);
        iconv_close(cd);
        return NULL;
    }

    *outptr = '\0';
    iconv_close(cd);
    return outbuf;
}


//// ------------------- โหลด PNG เป็น Bitmap ESC/POS (ลดขนาดได้) -------------------
//unsigned char* load_png_as_bitmap(const char *filename, int *width, int *height, int *bytes, float scale_factor) {
    //FILE *fp = fopen(filename, "rb");
    //if (!fp) return NULL;

    //png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    //if (!png_ptr) { fclose(fp); return NULL; }

    //png_infop info_ptr = png_create_info_struct(png_ptr);
    //if (!info_ptr) { png_destroy_read_struct(&png_ptr, NULL, NULL); fclose(fp); return NULL; }

    //if (setjmp(png_jmpbuf(png_ptr))) {
        //png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        //fclose(fp);
        //return NULL;
    //}

    //png_init_io(png_ptr, fp);
    //png_read_info(png_ptr, info_ptr);

    //int orig_width = png_get_image_width(png_ptr, info_ptr);
    //int orig_height = png_get_image_height(png_ptr, info_ptr);

    //png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    //png_byte bit_depth  = png_get_bit_depth(png_ptr, info_ptr);

    //if (bit_depth == 16) png_set_strip_16(png_ptr);
    //if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
    //if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
    //if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
    //if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY) png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
    //if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png_ptr);

    //png_read_update_info(png_ptr, info_ptr);

    //png_bytep *row_pointers = malloc(sizeof(png_bytep) * orig_height);
    //for (int y = 0; y < orig_height; y++)
        //row_pointers[y] = malloc(png_get_rowbytes(png_ptr, info_ptr));

    //png_read_image(png_ptr, row_pointers);
    //fclose(fp);

    //// ลดขนาดภาพตาม scale_factor
    //*width = orig_width * scale_factor;
    //*height = orig_height * scale_factor;

    //int bytes_per_line = ((*width + 7) / 8);
    //*bytes = bytes_per_line * (*height);
    //unsigned char *bitmap = calloc(*bytes, 1);

    //for (int y = 0; y < *height; y++) {
        //for (int x = 0; x < *width; x++) {
            //int src_x = x / scale_factor;
            //int src_y = y / scale_factor;
            //png_bytep px = &(row_pointers[src_y][src_x * 4]);
            //int gray = (px[0] + px[1] + px[2]) / 3;
            //if (gray < 128) {
                //int byte_index = (y * bytes_per_line) + (x / 8);
                //bitmap[byte_index] |= (0x80 >> (x % 8));
            //}
        //}
    //}

    //for (int y = 0; y < orig_height; y++) free(row_pointers[y]);
    //free(row_pointers);
    //png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    //return bitmap;
//}

// ------------------- โหลด PNG เป็น Bitmap ESC/POS พร้อมย่อภาพ -------------------
unsigned char* load_png_as_bitmap(const char *filename, int *width, int *height, int *bytes, float scale) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) { fclose(fp); return NULL; }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) { png_destroy_read_struct(&png_ptr, NULL, NULL); fclose(fp); return NULL; }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return NULL;
    }

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    int orig_width = png_get_image_width(png_ptr, info_ptr);
    int orig_height = png_get_image_height(png_ptr, info_ptr);

    png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    png_byte bit_depth  = png_get_bit_depth(png_ptr, info_ptr);

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY)
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    png_bytep *row_pointers = malloc(sizeof(png_bytep) * orig_height);
    for (int y = 0; y < orig_height; y++)
        row_pointers[y] = malloc(png_get_rowbytes(png_ptr, info_ptr));

    png_read_image(png_ptr, row_pointers);

    // ขนาดใหม่หลังย่อภาพ
    *width  = (int)(orig_width * scale);
    *height = (int)(orig_height * scale);

    int bytes_per_line = (*width + 7) / 8;
    *bytes = bytes_per_line * (*height);
    unsigned char *bitmap = calloc(*bytes, 1);

    // ย่อภาพแบบ nearest neighbor พร้อม weighted grayscale
    for (int y = 0; y < *height; y++) {
        for (int x = 0; x < *width; x++) {
            int orig_x = (int)(x / scale);
            int orig_y = (int)(y / scale);
            if (orig_x >= orig_width) orig_x = orig_width - 1;
            if (orig_y >= orig_height) orig_y = orig_height - 1;

            png_bytep px = &(row_pointers[orig_y][orig_x * 4]);
            int gray = (px[0] * 0.299 + px[1] * 0.587 + px[2] * 0.114);

            if (gray < 128) {
                int byte_index = y * bytes_per_line + x / 8;
                bitmap[byte_index] |= (0x80 >> (x % 8));
            }
        }
    }

    for (int y = 0; y < orig_height; y++)
        free(row_pointers[y]);
    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);

    return bitmap;
}

unsigned char* load_bitmap_corrected(const char *filename, int *width, int *height, int *bytes) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;

    // อ่าน header ของ BMP (54 ไบต์)
    unsigned char header[54];
    if (fread(header, 1, 54, fp) != 54) {
        fclose(fp);
        return NULL;
    }

    // ตรวจสอบว่าเป็นไฟล์ BMP
    if (header[0] != 'B' || header[1] != 'M') {
        fclose(fp);
        return NULL;
    }

    // อ่านความกว้างและความสูงของภาพ
    int w = *(int*)&header[18];
    int h = *(int*)&header[22];

    // จำนวนไบต์ต่อแถว (1 bit per pixel)
    int bytes_per_line = (w + 7) / 8;
    int data_size = bytes_per_line * h;

    unsigned char *bitmap_raw = malloc(data_size);
    if (!bitmap_raw) {
        fclose(fp);
        return NULL;
    }

    // ข้าม offset ไปยัง data bitmap
    int data_offset = *(int*)&header[10];
    fseek(fp, data_offset, SEEK_SET);

    // อ่าน bitmap raw
    fread(bitmap_raw, 1, data_size, fp);
    fclose(fp);

    // สร้าง bitmap ใหม่สำหรับเก็บภาพกลับหัวและ mirror
    unsigned char *bitmap_corrected = calloc(data_size, 1);
    if (!bitmap_corrected) {
        free(bitmap_raw);
        return NULL;
    }

    // ทำการกลับหัวและ mirror pixel
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int src_byte_index = y * bytes_per_line + (x / 8);
            int src_bit = 7 - (x % 8);

            int dst_x = w - 1 - x;  // mirror
            int dst_y = h - 1 - y;  // flip vertically
            int dst_byte_index = dst_y * bytes_per_line + (dst_x / 8);
            int dst_bit = 7 - (dst_x % 8);

            if (bitmap_raw[src_byte_index] & (1 << src_bit)) {
                bitmap_corrected[dst_byte_index] |= (1 << dst_bit);
            }
        }
    }

    free(bitmap_raw);

    *width = w;
    *height = h;
    *bytes = data_size;

    return bitmap_corrected;
}



// ------------------- พิมพ์ Bitmap ESC/POS -------------------
void escpos_print_bitmap(int fd, unsigned char *bitmap, int width, int height) {
    int bytes_per_line = (width + 7) / 8;
    unsigned char header[] = {
        0x1D, 0x76, 0x30, 0x00,
        bytes_per_line & 0xFF, (bytes_per_line >> 8) & 0xFF,
        height & 0xFF, (height >> 8) & 0xFF
    };
    write(fd, header, sizeof(header));
    write(fd, bitmap, bytes_per_line * height);
}

void escpos_print_bitmap_center(int fd, unsigned char *bitmap, int width, int height, int paper_width) {
    int bytes_per_line = (width + 7) / 8;
    int paper_bytes_per_line = (paper_width + 7) / 8;
    int offset_bytes = (paper_bytes_per_line - bytes_per_line) / 2;

    unsigned char header[] = {
        0x1D, 0x76, 0x30, 0x00,           // GS v 0 m
        bytes_per_line & 0xFF,            // xL
        (bytes_per_line >> 8) & 0xFF,     // xH
        height & 0xFF,                     // yL
        (height >> 8) & 0xFF               // yH
    };

    // พิมพ์ offset (padding) ถ้าจำเป็น
    if (offset_bytes > 0) {
        unsigned char *padding = calloc(offset_bytes * height, 1);
        unsigned char *new_bitmap = calloc(offset_bytes * height + bytes_per_line * height, 1);

        for (int y = 0; y < height; y++) {
            memcpy(new_bitmap + y * (offset_bytes + bytes_per_line), padding + y * offset_bytes, offset_bytes);
            memcpy(new_bitmap + y * (offset_bytes + bytes_per_line) + offset_bytes,
                   bitmap + y * bytes_per_line,
                   bytes_per_line);
        }
        free(padding);

        // อัปเดต header ให้ใช้ขนาดใหม่
        header[4] = (bytes_per_line + offset_bytes) & 0xFF;
        header[5] = ((bytes_per_line + offset_bytes) >> 8) & 0xFF;

        write(fd, header, sizeof(header));
        write(fd, new_bitmap, (bytes_per_line + offset_bytes) * height);
        free(new_bitmap);
    } else {
        write(fd, header, sizeof(header));
        write(fd, bitmap, bytes_per_line * height);
    }
}

void escpos_print_qrcode_scaled(int fd, QRcode *qrcode, int scale, int paper_width) {
    int qr_width = qrcode->width;
    int bytes_per_line = ((qr_width * scale) + 7) / 8;
    int paper_bytes_per_line = (paper_width + 7) / 8;
    int offset_bytes = (paper_bytes_per_line - bytes_per_line) / 2;

    unsigned char *bitmap = calloc(bytes_per_line * qr_width * scale, 1);
    if (!bitmap) return;

    for (int y = 0; y < qr_width; y++) {
        for (int x = 0; x < qr_width; x++) {
            if (qrcode->data[y * qr_width + x] & 0x01) {
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        int xx = x * scale + dx;
                        int yy = y * scale + dy;
                        bitmap[yy * bytes_per_line + (xx / 8)] |= (0x80 >> (xx % 8));
                    }
                }
            }
        }
    }

    int bytes_per_row = bytes_per_line;
    for (int y = 0; y < qr_width * scale; y++) {
        unsigned char header[] = {
            0x1B, 0x2A, 0x21, // Select bit-image mode
            bytes_per_row & 0xFF, (bytes_per_row >> 8) & 0xFF
        };
        write(fd, header, sizeof(header));

        // เติม offset เพื่อกึ่งกลาง
        for (int i = 0; i < offset_bytes; i++) {
            unsigned char zero = 0x00;
            write(fd, &zero, 1);
        }

        // พิมพ์ bitmap บรรทัดนั้น
        write(fd, bitmap + y * bytes_per_line, bytes_per_line);

        unsigned char nl = 0x0A; // newline
        write(fd, &nl, 1);
    }

    free(bitmap);
}

unsigned char* load_bitmap_fix(const char *filename, int *width, int *height, int *bytes) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;

    unsigned char header[54];
    if (fread(header, 1, 54, fp) != 54) {
        fclose(fp);
        return NULL;
    }

    if (header[0] != 'B' || header[1] != 'M') {
        fclose(fp);
        return NULL;
    }

    int w = *(int*)&header[18];
    int h = *(int*)&header[22];
    int data_offset = *(int*)&header[10];
    int bytes_per_line = (w + 7) / 8;
    int data_size = bytes_per_line * h;

    unsigned char *bitmap_raw = malloc(data_size);
    if (!bitmap_raw) {
        fclose(fp);
        return NULL;
    }

    fseek(fp, data_offset, SEEK_SET);
    fread(bitmap_raw, 1, data_size, fp);
    fclose(fp);

    unsigned char *bitmap_fixed = calloc(data_size, 1);
    if (!bitmap_fixed) {
        free(bitmap_raw);
        return NULL;
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int src_byte = y * bytes_per_line + (x / 8);
            int src_bit = x % 8;

            int dst_x = x;  // ไม่ mirror ซ้าย–ขวา
            int dst_y = h - 1 - y; // flip vertical
            int dst_byte = dst_y * bytes_per_line + (dst_x / 8);
            int dst_bit = dst_x % 8;

            // กลับสี: bit 0 → bit 1
            if (!(bitmap_raw[src_byte] & (1 << (7 - src_bit)))) {
                bitmap_fixed[dst_byte] |= (1 << (7 - dst_bit));
            }
        }
    }

    free(bitmap_raw);

    *width = w;
    *height = h;
    *bytes = data_size;
        printf("[DEBUG] Returned width: %d, height: %d, bytes: %d\n", *width, *height, *bytes);

    return bitmap_fixed;
}

bool is_valid_char(const unsigned char *p) {
    // ภาษาอังกฤษ a–z, A–Z
    if (isalpha(*p)) return true;

    // ตัวเลข 0–9
    if (isdigit(*p)) return true;

    // อักขระพิเศษทั่วไป (ASCII)
    const char *special = " .,'\"!?@#%&*^-_+=():;";
    if (strchr(special, *p)) return true;

    // ภาษาไทย UTF‑8: U+0E00..U+0E7F (3 ไบต์)
    if ((*p & 0xF0) == 0xE0 && (p[1] == 0xB8 || p[1] == 0xB9)) {
        return true;
    }

    return false;
}

bool remove_invalid_utf8(const char *input, char *output, size_t out_size) {
    size_t out_pos = 0;
    output[0] = '\0';

    const unsigned char *p = (const unsigned char *)input;

    while (*p && out_pos < out_size - 1) {
        if (is_valid_char(p)) {
            if ((*p & 0xF0) == 0xE0) { // ภาษาไทย (3 ไบต์)
                if (out_pos + 2 < out_size - 1) {
                    output[out_pos++] = *p++;
                    output[out_pos++] = *p++;
                    output[out_pos++] = *p++;
                } else {
                    break;
                }
            } else { // ASCII
                output[out_pos++] = *p++;
            }
        } else {
            // ข้าม emoji หรืออักขระพิเศษอื่น
            if ((*p & 0xF0) == 0xF0) p += 4;
            else if ((*p & 0xE0) == 0xE0) p += 3;
            else if ((*p & 0xC0) == 0xC0) p += 2;
            else p++;
        }
    }

    output[out_pos] = '\0';
    return true;
}


void print_slip_full(AppWidgets *app, Order *order) {
    if (!order) return;

    int fd = open(PRINTER_DEVICE, O_WRONLY);
    if (fd < 0) {
        perror("เปิดเครื่องพิมพ์ไม่ได้");
        return;
    }

    // เริ่มต้นเครื่องพิมพ์
    unsigned char init[] = {0x1B, 0x40};
    write(fd, init, sizeof(init));

    // ตั้ง Codepage เป็น PC874 (Codepage 30)
    unsigned char set_codepage[] = {0x1B, 0x74, 0x1E};
    write(fd, set_codepage, sizeof(set_codepage));

    char buf[512];
    char *txt;

    snprintf(buf, sizeof(buf), "ออเดอร์ #%d\n", order->order_id);
    txt = utf8_to_tis620(buf);
    if (txt) { write(fd, txt, strlen(txt)); free(txt); }

    //snprintf(buf, sizeof(buf), "ลูกค้า: %s\n", order->line_name);
    //txt = utf8_to_tis620(buf);
    //if (txt) { write(fd, txt, strlen(txt)); free(txt); }

    //snprintf(buf, sizeof(buf), "สถานที่ส่ง: %s\n", order->place);
    //txt = utf8_to_tis620(buf);
    //if (txt) { write(fd, txt, strlen(txt)); free(txt); }
    // เปิดตัวหนา
// ข้อความปกติ
snprintf(buf, sizeof(buf), "ลูกค้า: ");
txt = utf8_to_tis620(buf);
if (txt) { 
    write(fd, txt, strlen(txt)); 
    free(txt); 
}

// เปิดตัวหนา
unsigned char set_bold[] = {0x1B, 0x45, 0x01};
write(fd, set_bold, sizeof(set_bold));

// ชื่อ
snprintf(buf, sizeof(buf), "%s\n", order->line_name);
txt = utf8_to_tis620(buf);
if (txt) { 
    write(fd, txt, strlen(txt)); 
    free(txt); 
}

// ปิดตัวหนา
unsigned char reset_bold[] = {0x1B, 0x45, 0x00};
write(fd, reset_bold, sizeof(reset_bold));


// ข้อความปกติ
snprintf(buf, sizeof(buf), "สถานที่ส่ง: ");
txt = utf8_to_tis620(buf);
if (txt) { 
    write(fd, txt, strlen(txt)); 
    free(txt); 
}

// เปิดตัวหนา
write(fd, set_bold, sizeof(set_bold));

//// สถานที่ส่ง
//snprintf(buf, sizeof(buf), "%s\n", order->place);
//txt = utf8_to_tis620(buf);
//if (txt) { 
    //write(fd, txt, strlen(txt)); 
    //free(txt); 
//}

char clean_buf[512];
remove_invalid_utf8(order->place, clean_buf, sizeof(clean_buf));

//snprintf(buf, sizeof(buf), "%s\n", clean_buf);
snprintf(buf, sizeof(buf), "%.*s\n", (int)(sizeof(buf)-2), clean_buf);

txt = utf8_to_tis620(buf);
if (txt) { 
    write(fd, txt, strlen(txt)); 
    free(txt); 
}

// ปิดตัวหนา
write(fd, reset_bold, sizeof(reset_bold));

    snprintf(buf, sizeof(buf), "เวลาส่ง: %s\n", order->delivery_time);
    txt = utf8_to_tis620(buf);
    if (txt) { write(fd, txt, strlen(txt)); free(txt); }

    snprintf(buf, sizeof(buf), "วันที่สั่ง: %s\n", order->created_at);
    txt = utf8_to_tis620(buf);
    if (txt) { write(fd, txt, strlen(txt)); free(txt); }

    txt = utf8_to_tis620("รายการ:\n");
    if (txt) { write(fd, txt, strlen(txt)); free(txt); }

    double totalAmount = 0;
    for (int i = 0; i < order->item_count; i++) {
        snprintf(buf, sizeof(buf), "- %s %d x %.2f\n", order->items[i].name, order->items[i].qty, order->items[i].price);
        txt = utf8_to_tis620(buf);
        if (txt) { write(fd, txt, strlen(txt)); free(txt); }

        totalAmount += order->items[i].price * order->items[i].qty;
        if (strlen(order->items[i].option_text) > 0) {
            snprintf(buf, sizeof(buf), "  (%s)\n", order->items[i].option_text);
            txt = utf8_to_tis620(buf);
            if (txt) { write(fd, txt, strlen(txt)); free(txt); }
        }
    }

    snprintf(buf, sizeof(buf), "รวมทั้งหมด: %.2f บาท\n", totalAmount);
    txt = utf8_to_tis620(buf);
    if (txt) { write(fd, txt, strlen(txt)); free(txt); }
    
    // เพิ่มบรรทัดเว้นวรรคหลังโลโก้
    unsigned char nl = 0x0A; // new line
    for (int i = 0; i < 2; i++) { // เว้น 3 บรรทัด
        write(fd, &nl, 1);
    }

int logo_width, logo_height, logo_bytes;
//unsigned char *logo_bitmap = load_bitmap("PromptPay-logo.bmp", &logo_width, &logo_height, &logo_bytes);
//unsigned char *logo_bitmap = load_bitmap_corrected("PromptPay-logo.bmp", &logo_width, &logo_height, &logo_bytes);
unsigned char *logo_bitmap = load_bitmap_fix("PromptPay-logo.bmp", &logo_width, &logo_height, &logo_bytes);
//unsigned char *logo_bitmap = load_bitmap_fix("PromptPay-logo.bmp", &logo_width, &logo_height, &logo_bytes, 250);

if (logo_bitmap) {
    escpos_print_bitmap(fd, logo_bitmap, logo_width, logo_height);
    free(logo_bitmap);
    // เพิ่มบรรทัดเว้นวรรคหลังโลโก้
    unsigned char nl = 0x0A; // new line
    for (int i = 0; i < 2; i++) { // เว้น 3 บรรทัด
        write(fd, &nl, 1);
    }
}



    // -------- สร้าง QR PromptPay --------
    char amount_str[16] = "";
    if (totalAmount > 0) snprintf(amount_str, sizeof(amount_str), "%.2f", totalAmount);

    char *qrdata = build_promptpay_qr("0944574687", amount_str);
    if (!qrdata) {
        fprintf(stderr, "Failed to build QR data\n");
        close(fd);
        return;
    }

    QRcode *qrcode = QRcode_encodeString(qrdata, 0, QR_ECLEVEL_Q, QR_MODE_8, 3);
    free(qrdata);

    if (qrcode) {
        int qr_width = qrcode->width;
        int scale = 5; // ขยาย QR Code ขนาดขึ้น 3 เท่า
        int scaled_width = qr_width * scale;
        int bytes_per_line = (scaled_width + 7) / 8;
        int qr_bytes = bytes_per_line * scaled_width;
        unsigned char *qr_bitmap = calloc(qr_bytes, 1);

        for (int y = 0; y < qr_width; y++) {
            for (int x = 0; x < qr_width; x++) {
                if (qrcode->data[y * qr_width + x] & 1) {
                    for (int dy = 0; dy < scale; dy++) {
                        for (int dx = 0; dx < scale; dx++) {
                            int sx = x * scale + dx;
                            int sy = y * scale + dy;
                            int byte_index = sy * bytes_per_line + sx / 8;
                            qr_bitmap[byte_index] |= (0x80 >> (sx % 8));
                        }
                    }
                }
            }
        }
     
        // พิมพ์ QR Code แบบกึ่งกลาง
        escpos_print_bitmap_center(fd, qr_bitmap, scaled_width, scaled_width, 384);

        free(qr_bitmap);
        QRcode_free(qrcode);
        // เพิ่มบรรทัดเว้นวรรคหลังโลโก้
        unsigned char nl = 0x0A; // new line
        for (int i = 0; i < 2; i++) { // เว้น 3 บรรทัด
            write(fd, &nl, 1);
        }
    }
    


    // --- จัดกึ่งกลางข้อความหลัง QR Code ---
    unsigned char align_center[] = {0x1B, 0x61, 0x01}; // 0x01 = จัดกึ่งกลาง
    write(fd, align_center, sizeof(align_center));


    // พิมพ์เลข PromptPay
    snprintf(buf, sizeof(buf), "0944574687\n");
    txt = utf8_to_tis620(buf);
    if (txt) { write(fd, txt, strlen(txt)); free(txt); }

    // พิมพ์ชื่อบัญชี
    snprintf(buf, sizeof(buf), "บัญชี: โยธิน อินบรรเลง\n");
    txt = utf8_to_tis620(buf);
    if (txt) { write(fd, txt, strlen(txt)); free(txt); }

    // กลับไปจัดชิดซ้าย (ถ้าต้องการ)
    unsigned char align_left[] = {0x1B, 0x61, 0x00};
    write(fd, align_left, sizeof(align_left));

    unsigned char feed[] = {0x1B, 0x64, 0x05};  // feed 5 บรรทด
    write(fd, feed, sizeof(feed));

    close(fd);
}

