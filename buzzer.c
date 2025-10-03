#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

#define CHIP "/dev/gpiochip0"
#define LINE 6  // GPIO 6 (pin 7 บน header)

int main(void) {
    struct gpiod_chip *chip;
    struct gpiod_line *line;

    chip = gpiod_chip_open(CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        return 1;
    }

    line = gpiod_chip_get_line(chip, LINE);
    if (!line) {
        perror("gpiod_chip_get_line");
        gpiod_chip_close(chip);
        return 1;
    }

    if (gpiod_line_request_output(line, "buzzer", 0) < 0) {
        perror("gpiod_line_request_output");
        gpiod_chip_close(chip);
        return 1;
    }

    // 🔊 บี๊บยาว 1 วินาที
    gpiod_line_set_value(line, 1);
    sleep(1);
    gpiod_line_set_value(line, 0);

    usleep(300000); // พัก 300 ms ก่อนบี๊บสั้น

    // 🔊 บี๊บสั้นครั้งที่ 1 (50 ms)
    gpiod_line_set_value(line, 1);
    usleep(50000);   // 50 ms
    gpiod_line_set_value(line, 0);

    usleep(100000);  // พัก 100 ms ระหว่างสองบี๊บสั้น

    // 🔊 บี๊บสั้นครั้งที่ 2 (50 ms)
    gpiod_line_set_value(line, 1);
    usleep(50000);   // 50 ms
    gpiod_line_set_value(line, 0);

    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return 0;
}
