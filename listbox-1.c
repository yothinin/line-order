#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *listbox;
    gint selected_index;
} AppWidgets;

typedef struct {
    char *data;
    size_t size;
} MemoryStruct;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if(ptr == NULL) return 0;  // out of memory
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;
    return realsize;
}

static gchar* fetch_orders_json(const char *url) {
    CURL *curl = curl_easy_init();
    MemoryStruct chunk = {0};
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if(res != CURLE_OK) {
            g_printerr("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.data);
            return NULL;
        }
    }
    return chunk.data;
}

static void populate_listbox(AppWidgets *app, const gchar *json_data) {
    gtk_list_box_unselect_all(GTK_LIST_BOX(app->listbox));
    GError *error = NULL;
    JsonParser *parser = json_parser_new();
    if(!json_parser_load_from_data(parser, json_data, -1, &error)) {
        g_printerr("JSON parse error: %s\n", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    JsonObject *root = json_node_get_object(json_parser_get_root(parser));
    JsonArray *orders = json_object_get_array_member(root, "orders");
    guint n = json_array_get_length(orders);

    // Clear listbox
    GList *children, *iter;
    children = gtk_container_get_children(GTK_CONTAINER(app->listbox));
    for(iter = children; iter != NULL; iter = g_list_next(iter))
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    for(guint i = 0; i < n; i++) {
        JsonObject *order = json_array_get_object_element(orders, i);
        gint id = json_object_get_int_member(order, "id");
        const gchar *line_name = json_object_get_string_member(order, "line_name");
        const gchar *place = json_object_get_string_member(order, "place");
        const gchar *delivery_time = json_object_get_string_member(order, "delivery_time");
        const gchar *items_str = json_object_get_string_member(order, "items");

        // Parse items JSON string
        JsonParser *items_parser = json_parser_new();
        json_parser_load_from_data(items_parser, items_str, -1, NULL);
        JsonArray *items = json_node_get_array(json_parser_get_root(items_parser));

       
        GString *label_text = g_string_new(NULL);
        g_string_append_printf(label_text, "#%d %s | %s | %s\n", id, line_name, place, delivery_time);
        guint m = json_array_get_length(items);

        double total = 0.0;

        for(guint j = 0; j < m; j++) {
            JsonObject *item = json_array_get_object_element(items, j);
            const gchar *item_name = json_object_get_string_member(item, "item_name");
            int qty = json_object_get_int_member(item, "qty");
            const gchar *price_str = json_object_get_string_member(item, "price");
            double price = atof(price_str);
            const gchar *option_text = json_object_get_string_member(item, "option_text");

            // แสดงรายการหลัก
            g_string_append_printf(label_text, "\t-%s %d\n", item_name, qty);

            // แสดง option text ถ้ามี
            if(option_text && strlen(option_text) > 0)
                g_string_append_printf(label_text, "\t (%s)\n", option_text);

            total += qty * price;
        }

        // แสดงยอดรวมท้ายรายการ
        g_string_append_printf(label_text, "\t ยอดรวม %.2f\n", total);

        GtkWidget *label = gtk_label_new(label_text->str);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_label_set_yalign(GTK_LABEL(label), 0.5);
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

        GtkWidget *row = gtk_list_box_row_new();
        gtk_container_add(GTK_CONTAINER(row), label);
        gtk_list_box_insert(GTK_LIST_BOX(app->listbox), row, i);

        g_string_free(label_text, TRUE);
        g_object_unref(items_parser);
    }

    gtk_widget_show_all(app->listbox);

    // Restore selection
    if(app->selected_index >= 0 && app->selected_index < (gint)n) {
        GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), app->selected_index);
        gtk_list_box_select_row(GTK_LIST_BOX(app->listbox), row);
        gtk_widget_grab_focus(GTK_WIDGET(app->listbox)); // เพิ่มบรรทัดนี้
    }

    g_object_unref(parser);
}

//static void refresh_data(AppWidgets *app) {
    //const char *url = "https://api.maemoo.vip/api/admin/orders?lineId=U60f89193a53a22465d7bc2386096e1c9&date=2025-08-27";
    //gchar *json_data = fetch_orders_json(url);
    //if(json_data) {
        //populate_listbox(app, json_data);
        //free(json_data);
    //}
//}
static void refresh_data(AppWidgets *app) {
    time_t t = time(NULL);
    struct tm tm_now;
    localtime_r(&t, &tm_now);  // แปลงเวลาเป็นเวลาท้องถิ่น

    char date_str[11]; // "YYYY-MM-DD" + null
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", &tm_now);

    char url[256];
    snprintf(url, sizeof(url),
             "https://api.maemoo.vip/api/admin/orders?lineId=U60f89193a53a22465d7bc2386096e1c9&date=%s&status=0",
             date_str);

    gchar *json_data = fetch_orders_json(url);
    if(json_data) {
        populate_listbox(app, json_data);
        free(json_data);
    }
}

static void on_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;
    if(row)
        app->selected_index = gtk_list_box_row_get_index(row);
}

// Wrapper สำหรับ timeout
static gboolean refresh_data_timeout(gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;
    // จำ index ปัจจุบันก่อน refresh
    gint prev_index = app->selected_index;

    refresh_data(app);

    // กู้คืน selection หลัง refresh
    if(prev_index >= 0) {
        GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), prev_index);
        if(row)
            gtk_list_box_select_row(GTK_LIST_BOX(app->listbox), row);
    }

    return TRUE; // TRUE = ทำซ้ำทุก timeout
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    AppWidgets app;
    app.selected_index = -1;

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Kitchen Orders");
    gtk_window_fullscreen(GTK_WINDOW(app.window));
    gtk_widget_set_app_paintable(app.window, TRUE); // เพิ่มตรงนี้

    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(app.window), scrolled);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    app.listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scrolled), app.listbox);
    g_signal_connect(app.listbox, "row-selected", G_CALLBACK(on_row_selected), &app);

    // CSS
    GtkCssProvider *css = gtk_css_provider_new();
    GError *css_error = NULL;
    gtk_css_provider_load_from_data(css,
        "window, scrolledwindow, viewport, list box { background-color: #000000; }"
        "list row { background-color: #000000; }"
        "list row label { color: #666666; font-size: 64px; padding: 10px; }"
        "list row:selected { background-color: #333333; }"
        "list row:selected label { color: #ffffff; font-size: 64px; }",
        -1, &css_error);
    if(css_error) {
        g_printerr("CSS error: %s\n", css_error->message);
        g_error_free(css_error);
    }
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    refresh_data(&app);
    // ตั้ง timer ให้ refresh ทุก 5 วินาที
    g_timeout_add(5000, refresh_data_timeout, &app);

    gtk_widget_show_all(app.window);
    gtk_main();

    return 0;
}
