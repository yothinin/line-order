#include <iconv.h>
#include "utf8_conv.h"
#include "order_summary.h"
#include <json-glib/json-glib.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iconv.h>

typedef struct {
    AppWidgets *app;
    gchar *summary_text;
} PrintData;

char *remove_non_tis620(const char *utf8) {
    GString *out = g_string_new("");
    const char *p = utf8;
    gunichar ch;

    while (*p) {
        ch = g_utf8_get_char(p);
        if (ch < 0x80 || (ch >= 0x0E00 && ch <= 0x0E7F)) {
            g_string_append_unichar(out, ch);
        }
        p = g_utf8_next_char(p);
    }

    return g_string_free(out, FALSE);
}


static void print_summary(GtkWidget *widget, gpointer user_data) {
    PrintData *data = (PrintData *)user_data;
    AppWidgets *app = data->app;
    const char *summary_text = data->summary_text;

    FILE *printer = fopen(app->printer_device, "w");
    if (!printer) {
        g_printerr("❌ ไม่สามารถเปิดเครื่องพิมพ์ได้: %s\n", app->printer_device);
        return;
    }

    // ขยายตัวอักษร 2×2
    char size_cmd[] = {0x1D, 0x21, 0x11};
    fwrite(size_cmd, 1, sizeof(size_cmd), printer);

    char *clean_text = remove_non_tis620(summary_text);
    char *txt = utf8_to_tis620(clean_text);
    if (txt) {
        fwrite(txt, 1, strlen(txt), printer);
        free(txt);
    } else {
        g_printerr("❌ แปลงข้อความล้มเหลว\n");
    }

    // รีเซ็ตขนาดตัวอักษร
    char reset_cmd[] = {0x1D, 0x21, 0x00};
    fwrite(reset_cmd, 1, sizeof(reset_cmd), printer);

// เลื่อนบรรทัด 3 บรรทัด
char feed_cmd[] = {0x1B, 0x64, 0x03};
fwrite(feed_cmd, 1, sizeof(feed_cmd), printer);

    // ตัดกระดาษ
    char cut_cmd[] = {0x1D, 0x56, 0x00};
    fwrite(cut_cmd, 1, sizeof(cut_cmd), printer);

    fclose(printer);
    g_print("✅ ส่งสรุปยอดไปพิมพ์แล้ว\n");
}



static gboolean foreach_time_group(gpointer key, gpointer value, gpointer user_data) {
    GString *summary = (GString *)user_data;
    g_string_append_printf(summary, "%s : %d ออเดอร์\n", (char *)key, GPOINTER_TO_INT(value));
    return FALSE; // ไปต่อ
}

static gboolean foreach_menu_group(gpointer key, gpointer value, gpointer user_data) {
    GString *summary = (GString *)user_data;
    g_string_append_printf(summary, "%s : %s\n", (char *)key, (char *)value);
    return FALSE; // ไปต่อ
}

static gchar* parse_orders_and_generate_summary(const gchar *json_str, gchar **out_summary_text) {
    JsonParser *parser = json_parser_new();
    if (!json_parser_load_from_data(parser, json_str, -1, NULL)) {
        g_object_unref(parser);
        return NULL;
    }

    JsonNode *root = json_parser_get_root(parser);
    JsonObject *root_obj = json_node_get_object(root);
    JsonArray *orders_array = json_object_get_array_member(root_obj, "orders");

    GTree *time_groups = g_tree_new((GCompareFunc)g_strcmp0);
    GTree *menu_groups = g_tree_new((GCompareFunc)g_strcmp0);

    double notPaid = 0, transfer = 0, cash = 0;

    for (guint i = 0; i < json_array_get_length(orders_array); i++) {
        JsonObject *order = json_array_get_object_element(orders_array, i);

        int cancelled = json_object_get_int_member(order, "cancelled");
        if (cancelled != 0) continue;

        const gchar *delivery_time = json_object_get_string_member(order, "delivery_time");
        gpointer prev_count = g_tree_lookup(time_groups, delivery_time);
        g_tree_insert(time_groups, g_strdup(delivery_time), GINT_TO_POINTER(GPOINTER_TO_INT(prev_count) + 1));

        const gchar *items_str = json_object_get_string_member(order, "items");
        JsonParser *items_parser = json_parser_new();
        json_parser_load_from_data(items_parser, items_str, -1, NULL);
        JsonArray *items_array = json_node_get_array(json_parser_get_root(items_parser));

        double order_total = 0;
        for (guint j = 0; j < json_array_get_length(items_array); j++) {
            JsonObject *item = json_array_get_object_element(items_array, j);
            const gchar *item_name = json_object_get_string_member(item, "item_name");
            int qty = json_object_get_int_member(item, "qty");

            const gchar *price_str = json_object_get_string_member(item, "price");
            double price = atof(price_str);
            order_total += qty * price;

            gchar *prev_qty_str = g_tree_lookup(menu_groups, item_name);
            int prev_qty = prev_qty_str ? atoi(prev_qty_str) : 0;
            gchar buf[32];
            snprintf(buf, sizeof(buf), "%d", prev_qty + qty);
            g_tree_insert(menu_groups, g_strdup(item_name), g_strdup(buf));
        }
        g_object_unref(items_parser);

        int status = json_object_get_int_member(order, "status");
        if (status < 4) notPaid += order_total;
        else if (status == 4) transfer += order_total;
        else if (status == 5) cash += order_total;
    }

    // สร้างสรุป
    GString *summary = g_string_new("=สรุปยอดออเดอร์\n\n");

    // สรุปตามเวลาส่ง
    g_string_append(summary, "🕒 สรุปตามเวลาส่ง\n");
    g_tree_foreach(time_groups, foreach_time_group, summary);
    g_string_append(summary, "\n");

    // สรุปเมนูเรียงชื่อเมนู
    g_string_append(summary, "🍽 สรุปตามเมนู\n");
    g_tree_foreach(menu_groups, foreach_menu_group, summary);
    g_string_append(summary, "\n");

    //// สรุปการชำระเงิน
    //g_string_append(summary, "💰 สรุปการชำระเงิน\n");
    //g_string_append_printf(summary, "ยังไม่ชำระเงิน: %.2f บาท\n", notPaid);
    //g_string_append_printf(summary, "โอนเงิน: %.2f บาท\n", transfer);
    //g_string_append_printf(summary, "เงินสด: %.2f บาท\n", cash);

    g_tree_destroy(time_groups);
    g_tree_destroy(menu_groups);

    *out_summary_text = g_string_free(summary, FALSE);
    g_object_unref(parser);
    return *out_summary_text;
}

void show_order_summary(GtkWidget *widget, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "สรุปยอดออเดอร์");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), text_view, TRUE, TRUE, 0);

    GtkWidget *btn_print = gtk_button_new_with_label("🖨 พิมพ์");
    gtk_box_pack_start(GTK_BOX(vbox), btn_print, FALSE, FALSE, 5);

    gchar url[512];
    snprintf(url, sizeof(url),
        "%s/api/admin/orders?lineId=%s&date=%s",
        app->api_base_url, app->line_id, app->selected_date);

    g_print("URL: %s\n", url);

    gchar *json_str = fetch_orders_json(url);
    if (!json_str) {
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        gtk_text_buffer_set_text(buffer, "❌ โหลดข้อมูลไม่สำเร็จ", -1);
        return;
    }

    gchar *summary_text = NULL;
    parse_orders_and_generate_summary(json_str, &summary_text);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, summary_text, -1);

PrintData *pdata = g_malloc(sizeof(PrintData));
pdata->app = app;
pdata->summary_text = summary_text;

g_signal_connect(btn_print, "clicked", G_CALLBACK(print_summary), pdata);

    gtk_widget_show_all(window);
    g_free(json_str);
}
