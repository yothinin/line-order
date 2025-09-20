#include "screenfade.h"

static gboolean destroy_window_cb(gpointer user_data) {
    gtk_widget_destroy(GTK_WIDGET(user_data));
    return FALSE; // ให้ timeout ถูกลบอัตโนมัติ
}

void start_screen_fade(GtkWindow *parent) {
    // สร้าง window overlay สำหรับ fade
    GtkWidget *fade_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_transient_for(GTK_WINDOW(fade_win), parent);
    gtk_window_set_decorated(GTK_WINDOW(fade_win), FALSE);
    gtk_window_set_modal(GTK_WINDOW(fade_win), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(fade_win), 400, 300); // ปรับขนาดตามต้องการ
    gtk_window_set_position(GTK_WINDOW(fade_win), GTK_WIN_POS_CENTER_ON_PARENT);

    // CSS ให้พื้นหลังดำ
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: rgba(0,0,0,1.0); }", -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(fade_win);
    gtk_style_context_add_provider(context,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);

    gtk_widget_show_all(fade_win);

    // ทำลาย window หลัง 2 วินาที
    g_timeout_add(2000, destroy_window_cb, fade_win);
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
