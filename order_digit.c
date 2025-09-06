#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ฟังก์ชันคำนวณ Check Digit แบบถ่วงน้ำหนัก (mod 10)
int calc_check_digit_weighted(const char *order_str) {
    int sum = 0;
    int weights[] = {3, 1, 7, 9, 3, 1, 7};
    int len = strlen(order_str);

    for (int i = 0; i < len; i++) {
        int digit = order_str[i] - '0';
        sum += digit * weights[i % 7];
    }
    return sum % 10;
}

// ฟังก์ชันสร้าง order + check digit
void generate_order_with_check_digit(int order_id, char *output, size_t out_size) {
    char order_str[20];
    snprintf(order_str, sizeof(order_str), "%d", order_id); // ไม่มี zero-padding

    int check_digit = calc_check_digit_weighted(order_str);

    // รวม order + check digit
    snprintf(output, out_size, "%s%d", order_str, check_digit);
}

// Verify แบบใช้ความยาวจริง
int verify_order_with_check_digit(const char *order_with_cd) {
    size_t len = strlen(order_with_cd);
    if (len < 2) return 0; // ต้องมีอย่างน้อย 1 หลัก + check digit

    char order_str[20];
    strncpy(order_str, order_with_cd, len - 1);
    order_str[len - 1] = '\0';

    int expected_cd = calc_check_digit_weighted(order_str);
    int given_cd = order_with_cd[len - 1] - '0';

    return expected_cd == given_cd;
}

int main(int argc, char *argv[]) {
    char order_with_cd[20];

    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s gen <order_number>\n", argv[0]);
        printf("  %s verify <order_code>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "gen") == 0 && argc == 3) {
        int order_id = atoi(argv[2]);
        generate_order_with_check_digit(order_id, order_with_cd, sizeof(order_with_cd));
        printf("Full Order Code: %s\n", order_with_cd);
    }
    else if (strcmp(argv[1], "verify") == 0 && argc == 3) {
        if (verify_order_with_check_digit(argv[2])) {
            printf("Order code %s is VALID ✅\n", argv[2]);
        } else {
            printf("Order code %s is INVALID ❌\n", argv[2]);
        }
    }
    else {
        printf("Invalid usage.\n");
    }

    return 0;
}
