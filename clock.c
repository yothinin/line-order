#include "clock.h"
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>

static ClockData clock_data;
static pthread_t clock_thread;

// ฟังก์ชันเรียกบน main thread
static gboolean update_clock_label_idle(gpointer data) {
    char *str = (char *)data;
    gtk_label_set_text(GTK_LABEL(clock_data.clock_label), str);
    g_free(str); // free memory หลังใช้งาน
    return G_SOURCE_REMOVE; // เรียกครั้งเดียว
}

// ฟังก์ชัน thread
static void* clock_thread_func(void *arg) {
    while (clock_data.running) {
        time_t t = time(NULL);
        struct tm tm_now;
        localtime_r(&t, &tm_now);
        char buf[9]; // HH:MM:SS
        strftime(buf, sizeof(buf), "%H:%M", &tm_now);

        // ส่งข้อความไป main thread
        gdk_threads_add_idle_full(G_PRIORITY_DEFAULT_IDLE,
                                  update_clock_label_idle,
                                  g_strdup(buf), // ต้องใช้ g_strdup เพราะฟังก์ชัน idle ใช้ pointer
                                  NULL);

        sleep(1);
    }
    return NULL;
}

// เริ่ม thread
void start_clock_thread(GtkLabel *label) {
    clock_data.clock_label = label;
    clock_data.running = TRUE;
    pthread_create(&clock_thread, NULL, clock_thread_func, NULL);
}

// หยุด thread
void stop_clock_thread() {
    clock_data.running = FALSE;
    pthread_join(clock_thread, NULL);
}
