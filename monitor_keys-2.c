#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <termios.h>

#define SERVER_NAME "orangepione02.local"  // ใช้ชื่อ host แทน IP
#define SERVER_PORT 5000
#define BUF_SIZE 32

int sockfd;

// จัดการเมื่อกด Ctrl+C
void intHandler(int dummy) {
    if (sockfd > 0) close(sockfd);
    printf("\nExiting client.\n");
    exit(0);
}

// ตั้ง terminal เป็น raw mode
struct termios orig_term;
void enableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_term);
    raw = orig_term;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
}

// ฟังก์ชันแปลง hostname → IP
int resolve_hostname(const char *hostname, char *ipstr, size_t size) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // IPv4
    hints.ai_socktype = SOCK_DGRAM;

    int status = getaddrinfo(hostname, NULL, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "Error resolving %s: %s\n", hostname, gai_strerror(status));
        return -1;
    }

    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &(ipv4->sin_addr), ipstr, size);
    freeaddrinfo(res);
    return 0;
}

int main() {
    signal(SIGINT, intHandler);

    char server_ip[INET_ADDRSTRLEN];
    if (resolve_hostname(SERVER_NAME, server_ip, sizeof(server_ip)) != 0) {
        fprintf(stderr, "Could not resolve server hostname.\n");
        return 1;
    }

    printf("Resolved %s → %s\n", SERVER_NAME, server_ip);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket failed");
        return 1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return 1;
    }

    enableRawMode();
    atexit(disableRawMode);

    printf("UDP Monitor Client connected to %s (%s:%d)\n", SERVER_NAME, server_ip, SERVER_PORT);
    printf("Controls:\n");
    printf("  1: m1, 2: m2, 3: m3, ESC, TAB, ↑↓←→, ENTER, SPACE\n");
    printf("  Ctrl+Alt+X = shutdown, Ctrl+Alt+R = reboot\n");
    printf("  q: quit\n");

    char buf[BUF_SIZE];
    while (1) {
        unsigned char c;
        if (read(STDIN_FILENO, &c, 1) != 1) continue;

        memset(buf, 0, sizeof(buf));

        // ปุ่ม q ออกจากโปรแกรม
        if (c == 'q') break;

        // ตรวจจับปุ่มปกติ
        else if (c == '1') strncpy(buf, "m1", BUF_SIZE);
        else if (c == '2') strncpy(buf, "m2", BUF_SIZE);
        else if (c == '3') strncpy(buf, "m3", BUF_SIZE);
        else if (c == 'd') strncpy(buf, "do", BUF_SIZE);
        else if (c == 'o') strncpy(buf, "done", BUF_SIZE);
        else if (c == '\t') strncpy(buf, "tab", BUF_SIZE);
        else if (c == '\n' || c == '\r') strncpy(buf, "enter", BUF_SIZE);
        else if (c == ' ') strncpy(buf, "space", BUF_SIZE);

        // ตรวจจับ ESC / ALT / Ctrl+Alt
        else if (c == 27) {
            unsigned char seq;
            if (read(STDIN_FILENO, &seq, 1) != 1) continue;

            // ✅ Ctrl+Alt+X (ESC + 24)
            if (seq == 24) {
                strncpy(buf, "shutdown", BUF_SIZE);
            }
            // ✅ Ctrl+Alt+R (ESC + 18)
            else if (seq == 18) {
                strncpy(buf, "reboot", BUF_SIZE);
            }
            // ลูกศร หรือ Shift+Tab
            else if (seq == '[') {
                unsigned char arrow;
                if (read(STDIN_FILENO, &arrow, 1) != 1) continue;
                if (arrow == 'A') strncpy(buf, "up", BUF_SIZE);
                else if (arrow == 'B') strncpy(buf, "down", BUF_SIZE);
                else if (arrow == 'C') strncpy(buf, "right", BUF_SIZE);
                else if (arrow == 'D') strncpy(buf, "left", BUF_SIZE);
                else if (arrow == 'Z') strncpy(buf, "shift_tab", BUF_SIZE);
                else continue;
            }
            // Alt + a..z
            else if (seq >= 'a' && seq <= 'z') {
                snprintf(buf, BUF_SIZE, "alt_%c", seq);
            }
            else {
                strncpy(buf, "esc", BUF_SIZE);
            }
        } else continue;

        // ส่งข้อมูลผ่าน UDP
        if (sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            perror("Send failed");
        else
            printf("[Sent] %s\n", buf);
    }

    close(sockfd);
    return 0;
}
