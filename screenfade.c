#include "screenfade.h"

// ซ่อนและลบ window หลัง 2 วินาที
static gboolean destroy_window_cb(gpointer user_data) {
    GtkWidget *window = GTK_WIDGET(user_data);
    gtk_widget_destroy(window);
    return FALSE;
}

// สร้างหน้าต่างสีดำเต็มจอ 2 วินาที
void start_screen_fade(GtkWindow *parent) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_fullscreen(GTK_WINDOW(window));
    gtk_widget_override_background_color(window, GTK_STATE_FLAG_NORMAL, &(GdkRGBA){0,0,0,1.0});
    gtk_window_present(GTK_WINDOW(window));

    // destroy หลัง 2 วินาที
    g_timeout_add(2000, destroy_window_cb, window);
}

// timeout callback เรียก start_screen_fade
static gboolean screen_fade_timeout_cb(gpointer user_data) {
    GtkWindow *window = GTK_WINDOW(user_data);
    start_screen_fade(window);
    return TRUE; // เรียกซ้ำทุก interval
}

// setup auto fade
void setup_auto_screen_fade(GtkWindow *window, guint interval_ms) {
    g_timeout_add(interval_ms, screen_fade_timeout_cb, window);
}
