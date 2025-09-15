#include "listbox.h"
#include "screenfade.h"
#include "clock.h"
#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "slip.h"

const char *STATUS_NAMES[]  = {"รับออเดอร์","กำลังทำอาหาร","กำลังจัดส่ง","จัดส่งแล้ว","ชำระเงินแล้ว"};
const char *STATUS_COLORS[] = {"green","orange","red","blue","purple"};

gboolean is_dark_theme() {
    GtkSettings *settings = gtk_settings_get_default();
    gboolean dark_theme = FALSE;
    g_object_get(settings, "gtk-application-prefer-dark-theme", &dark_theme, NULL);
    return dark_theme;
}

static void set_field(char *dest, size_t size, const char *value) {
    strncpy(dest, value, size - 1);
    dest[size - 1] = '\0';
}

void load_dotenv_to_struct(AppWidgets *app, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = line;
        char *value = eq + 1;

        while (*key == ' ') key++;
        while (*value == ' ') value++;

        if (strcmp(key, "MACHINE_NAME") == 0)
            set_field(app->machine_name, sizeof(app->machine_name), value);
        else if (strcmp(key, "TOKEN") == 0)
            set_field(app->token, sizeof(app->token), value);
        else if (strcmp(key, "API_BASE_URL") == 0)
            set_field(app->api_base_url, sizeof(app->api_base_url), value);
        else if (strcmp(key, "MONITOR") == 0)
            app->selected_monitor = atoi(value);  // แปลงค่าเป็น int
    }
 
    fclose(f);
}

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if(ptr == NULL) return 0;
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;
    return realsize;
}

void select_first_row(AppWidgets *app) {
    GtkListBoxRow *first_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), 0);
    if (first_row) {
        gtk_list_box_select_row(GTK_LIST_BOX(app->listbox), first_row);
        gtk_widget_grab_focus(GTK_WIDGET(first_row));
    }
}

gchar* fetch_orders_json(const char *url) {
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


// ฟังก์ชันส่งอัพเดท status ไป API
void update_order_status(AppWidgets *app, gint order_id, gint status) {
    CURL *curl = curl_easy_init();
    if(!curl) return;

    const char *machine_name = app->machine_name;
    const char *token = app->token;

    if(!machine_name || !token) {
        g_printerr("❌ MACHINE_NAME หรือ TOKEN ไม่ถูกตั้งค่าใน struct\n");
        return;
    }

    char url[1024];
    snprintf(url, sizeof(url), "%s/api/store/orders/%d/update_status",
         app->api_base_url, order_id);


    // สร้าง JSON payload
    char postfields[512];
    snprintf(postfields, sizeof(postfields),
             "{\"status\":%d,\"machine_name\":\"%s\",\"token\":\"%s\"}",
             status, machine_name, token);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK)
        g_printerr("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

void update_order_canceled(AppWidgets *app, int order_id, int canceled) {
    const char *machine_name = app->machine_name;
    const char *token = app->token;

    if(!machine_name || !token) {
        g_printerr("❌ MACHINE_NAME หรือ TOKEN ไม่ถูกตั้งค่าใน struct\n");
        return;
    }

    CURL *curl = curl_easy_init();
    if (!curl) return;

    char url[1024];
    snprintf(url, sizeof(url), "%s/api/store/orders/%d/cancel", app->api_base_url, order_id);

    // สร้าง JSON payload
    char postfields[512];
    snprintf(postfields, sizeof(postfields),
             "{\"cancelled\":%d,\"machine_name\":\"%s\",\"token\":\"%s\"}",
             canceled, machine_name, token);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        g_warning("Curl failed: %s", curl_easy_strerror(res));
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

// ตัวอย่าง populate_listbox
void populate_listbox(AppWidgets *app, const gchar *json_data) {
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

    // ลบ row เก่า
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->listbox));
    for(GList *iter = children; iter != NULL; iter = g_list_next(iter))
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    for(guint i = 0; i < n; i++) {
        JsonObject *order = json_array_get_object_element(orders, i);
        gint id = json_object_get_int_member(order, "id");
        const gchar *line_name = json_object_get_string_member(order, "line_name");
        
        const gchar *line_id = json_object_get_string_member(order, "line_id");

        const gchar *place = json_object_get_string_member(order, "place");
        const gchar *delivery_time = json_object_get_string_member(order, "delivery_time");
        gint status = json_object_get_int_member(order, "status");

        const gchar *items_str = json_object_get_string_member(order, "items");

        JsonParser *items_parser = json_parser_new();
        json_parser_load_from_data(items_parser, items_str, -1, NULL);
        JsonArray *items = json_node_get_array(json_parser_get_root(items_parser));

        const gchar *status_text = "";
        if(status > 0 && status < 5) {
            status_text = STATUS_NAMES[status];
        }

        GString *label_text = g_string_new(NULL);
        g_string_append_printf(label_text, "#%d %s | %s | %s %s\n", id, line_name, place, delivery_time, status_text);

        guint m = json_array_get_length(items);
        double total = 0.0;
        for(guint j = 0; j < m; j++) {
            JsonObject *item = json_array_get_object_element(items, j);
            const gchar *item_name = json_object_get_string_member(item, "item_name");
            int qty = json_object_get_int_member(item, "qty");
            const gchar *price_str = json_object_get_string_member(item, "price");
            double price = atof(price_str);
            const gchar *option_text = json_object_get_string_member(item, "option_text");

            g_string_append_printf(label_text, "\t-%s %d\n", item_name, qty);
            if(option_text && strlen(option_text) > 0)
                g_string_append_printf(label_text, "\t (%s)\n", option_text);

            total += qty * price;
        }
        g_string_append_printf(label_text, "\t ยอดรวม %.2f\n", total);

        GtkWidget *label = gtk_label_new(label_text->str);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_label_set_yalign(GTK_LABEL(label), 0.5);
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

        GtkWidget *row = gtk_list_box_row_new();
        g_object_set_data(G_OBJECT(row), "status", GINT_TO_POINTER(status));

        gtk_container_add(GTK_CONTAINER(row), label);

        g_object_set_data_full(G_OBJECT(row), "line_id", g_strdup(line_id), g_free);

        gtk_list_box_insert(GTK_LIST_BOX(app->listbox), row, i);

        g_string_free(label_text, TRUE);
        g_object_unref(items_parser);
    }

    gtk_widget_show_all(app->listbox);

    if(app->selected_index >= 0 && app->selected_index < (gint)n) {
        GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), app->selected_index);
        gtk_list_box_select_row(GTK_LIST_BOX(app->listbox), row);
        gtk_widget_grab_focus(GTK_WIDGET(app->listbox));
    }

    g_object_unref(parser);
}


//void refresh_data(AppWidgets *app) {
    //char date_str[11] = {0};   // init ให้ null-terminated ตั้งแต่แรก

    //// ถ้ามี filter_date ใช้ค่า ถ้าไม่ใช้วันปัจจุบัน
    //if (strlen(app->filter_date) > 0) {
        //strncpy(date_str, app->filter_date, sizeof(date_str) - 1);
        //date_str[sizeof(date_str) - 1] = '\0';
        //date_str[strcspn(date_str, "\r\n")] = '\0'; // ตัด newline เผื่อมี
    //} else {
        //time_t t = time(NULL);
        //struct tm tm_now;
        //localtime_r(&t, &tm_now);
        //strftime(date_str, sizeof(date_str), "%Y-%m-%d", &tm_now);
    //}

    //char url[1024];
    //snprintf(url, sizeof(url),
             //"%s/api/store/orders?date=%s",
             //app->api_base_url, date_str);

////    printf("🔍 Final fetch URL = [%s]\n", url); // debug

    //gchar *json_data = fetch_orders_json(url);
    //if(json_data) {
        //populate_listbox(app, json_data);
        //free(json_data);
        //select_first_row(app);
    //}
//}

void refresh_data(AppWidgets *app) {
    char date_str[11] = {0};

    if (strlen(app->filter_date) > 0) {
        strncpy(date_str, app->filter_date, sizeof(date_str) - 1);
        date_str[sizeof(date_str) - 1] = '\0';
        date_str[strcspn(date_str, "\r\n")] = '\0';
    } else {
        time_t t = time(NULL);
        struct tm tm_now;
        localtime_r(&t, &tm_now);
        strftime(date_str, sizeof(date_str), "%Y-%m-%d", &tm_now);
    }

    char url[1024];
    snprintf(url, sizeof(url),
             "%s/api/store/orders?date=%s&monitor=%d",
             app->api_base_url, date_str,
             (app->selected_monitor > 0 ? app->selected_monitor : 1));

    gchar *json_data = fetch_orders_json(url);
    if (json_data) {
        populate_listbox(app, json_data);
        free(json_data);
        select_first_row(app);
    }
}


void refocus_selected_row(AppWidgets *app) {
    if (app->selected_index < 0) return;

    GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), app->selected_index);
    if (row) {
        // เลือก row เดิม
        gtk_list_box_select_row(GTK_LIST_BOX(app->listbox), row);

        // เอา focus กลับไปที่ row
        gtk_widget_grab_focus(GTK_WIDGET(row));

        // เลื่อน scroll ให้ row นี้อยู่ด้านบน
        scroll_listbox_to_row(app, app->selected_index);
    }
}


// callback ปุ่ม "ทำรายการ"
void btn_do_clicked_cb(GtkButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;

    if(app->selected_index < 0) return;

    GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), app->selected_index);
    if(!row) return;

    // ดึง order_id จาก label
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
    const gchar *text = gtk_label_get_text(GTK_LABEL(label));
    gint order_id = 0;
    sscanf(text, "#%d", &order_id);

    g_print ("ทำรายการ... %d\n", order_id);
    
    gint prev_index = app->selected_index;

    // เรียก update_order_status ส่ง status = 1
    if(order_id > 0)
        update_order_status(app, order_id, 1);

    // รีเฟรช listbox ใหม่
    refresh_data(app);
    
    // 🔹 restore row เดิม
    GtkListBoxRow *new_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), prev_index);
    if(new_row) {
        gtk_list_box_select_row(GTK_LIST_BOX(app->listbox), new_row);
        gtk_widget_grab_focus(GTK_WIDGET(new_row));
        app->selected_index = prev_index; // อัปเดต index ใหม่
    }
    
    refocus_selected_row(app);
}


void on_row_selected(GtkListBox *box, GtkListBoxRow *row, AppWidgets *app) {
    if (!row) {
        app->selected_index = -1;
        app->selected_order_id = -1;
        gtk_widget_set_sensitive(app->btn_do, FALSE);
        gtk_widget_set_sensitive(app->btn_done, FALSE);
        return;
    }

    app->selected_index = gtk_list_box_row_get_index(row);

    // ดึง order_id จาก label
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
    const gchar *text = gtk_label_get_text(GTK_LABEL(label));
    sscanf(text, "#%d", &app->selected_order_id);

    // กำหนดปุ่ม enable/disable ตาม status
    gint status = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "status"));
    if (status == 0) {
        gtk_widget_set_sensitive(app->btn_do, TRUE);
        gtk_widget_set_sensitive(app->btn_done, FALSE);
    } else if (status == 1) {
        gtk_widget_set_sensitive(app->btn_do, FALSE);
        gtk_widget_set_sensitive(app->btn_done, TRUE);
    } else {
        gtk_widget_set_sensitive(app->btn_do, FALSE);
        gtk_widget_set_sensitive(app->btn_done, FALSE);
    }
}


gboolean refresh_data_timeout(gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;
    gint prev_index = app->selected_index;
    refresh_data(app);
    if(prev_index >= 0) {
        GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), prev_index);
        if(row)
            gtk_list_box_select_row(GTK_LIST_BOX(app->listbox), row);
    }
    return TRUE;
}

// อัพเดทนาฬิกา
gboolean update_clock(gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;
    time_t t = time(NULL);
    struct tm tm_now;
    localtime_r(&t, &tm_now);
    char buf[9]; // HH:MM:SS
    strftime(buf, sizeof(buf), "%H:%M:%S", &tm_now);
    gtk_label_set_text(GTK_LABEL(app->clock_label), buf);
    return TRUE;
}


void scroll_listbox_to_row(AppWidgets *app, gint idx) {
    if (idx < 0) return;

    GtkListBox *box = GTK_LIST_BOX(app->listbox);
    GtkListBoxRow *row = gtk_list_box_get_row_at_index(box, idx);
    if (!row) return;

    // คำนวณ y offset ของ row
    gint y = 0;
    GList *children = gtk_container_get_children(GTK_CONTAINER(box));
    for (GList *l = children; l != NULL; l = l->next) {
        if (l->data == row) break;
        GtkAllocation alloc;
        gtk_widget_get_allocation(GTK_WIDGET(l->data), &alloc);
        y += alloc.height;
    }
    g_list_free(children);

    // เลื่อน scroll ของ scrolled window
    GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(app->scrolled));
    gtk_adjustment_set_value(adj, y);
    gtk_widget_queue_draw(GTK_WIDGET(app->scrolled));
}


void scroll_listbox_up_cb(GtkButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;
    if(app->selected_index > 0) {
        app->selected_index--;
        gtk_list_box_select_row(GTK_LIST_BOX(app->listbox),
            gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), app->selected_index));
        scroll_listbox_to_row(app, app->selected_index);
    }
}

void scroll_listbox_down_cb(GtkButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->listbox));
    gint n = g_list_length(children);
    g_list_free(children);

    if(app->selected_index < n - 1) {
        app->selected_index++;
        gtk_list_box_select_row(GTK_LIST_BOX(app->listbox),
            gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), app->selected_index));
        scroll_listbox_to_row(app, app->selected_index);
    }
}

void btn_done_clicked_cb(GtkButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;

    if (app->selected_index < 0) return;

    GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), app->selected_index);
    if (!row) return;

    // ตรวจสอบสถานะปัจจุบัน
    gint status = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "status"));
    if (status != 1) {
        g_print("ไม่สามารถเปลี่ยนสถานะเป็น 2 ได้ เพราะสถานะปัจจุบันไม่ใช่ 1\n");
        return;
    }

    // ดึง order_id จาก label
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
    const gchar *text = gtk_label_get_text(GTK_LABEL(label));
    gint order_id = 0;
    sscanf(text, "#%d", &order_id);
    if (order_id <= 0) return;

    // ส่งไป API /api/store/orders/:id/update_status
    CURL *curl = curl_easy_init();
    if (curl) {
        char url[1024];
        snprintf(url, sizeof(url), "%s/api/store/orders/%d/update_status",
                 app->api_base_url, order_id);

        char postfields[512];
        snprintf(postfields, sizeof(postfields),
                 "{\"status\":2,\"machine_name\":\"%s\",\"token\":\"%s\"}",
                 app->machine_name, app->token);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            g_printerr("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    // ดึง order ล่าสุดจาก API และพิมพ์สลิป
    Order *order = get_order_by_id(app->api_base_url, app->machine_name, app->token, order_id);
    if (order) {
        print_slip_full(order);
        g_free(order);
    }

    // รีเฟรช listbox ใหม่
    refresh_data(app);
    refocus_selected_row(app);
}


void btn_cancel_clicked_cb(GtkButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;

    int order_id = app->selected_order_id;

    // สร้างข้อความยืนยันพร้อม order_id
    gchar *message = g_strdup_printf("กรุณายืนยันยกเลิกออเดอร์ #%d ?", order_id);

    // สร้าง dialog
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_WARNING,
                                               GTK_BUTTONS_NONE,
                                               "%s", message);

    // โหลด CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style.css", NULL);

    // =========================
    // ปุ่ม Yes (ยืนยันยกเลิก) สีเขียว ✔️
    // =========================
    GtkWidget *btn_yes = gtk_button_new();
    GtkWidget *hbox_yes = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *icon_yes = gtk_image_new_from_icon_name("emblem-ok", GTK_ICON_SIZE_BUTTON);
    GtkWidget *label_yes = gtk_label_new("ยืนยัน");

    gtk_box_pack_start(GTK_BOX(hbox_yes), icon_yes, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_yes), label_yes, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(btn_yes), hbox_yes);

    GtkStyleContext *context = gtk_widget_get_style_context(btn_yes);
    gtk_style_context_add_provider(context,
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_add_class(context, "yes-button");

    // =========================
    // ปุ่ม No (ยกเลิก) สีแดง ✖️
    // =========================
    GtkWidget *btn_no = gtk_button_new();
    GtkWidget *hbox_no = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *icon_no = gtk_image_new_from_icon_name("process-stop", GTK_ICON_SIZE_BUTTON);
    GtkWidget *label_no = gtk_label_new("ยกเลิก");

    gtk_box_pack_start(GTK_BOX(hbox_no), icon_no, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_no), label_no, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(btn_no), hbox_no);

    context = gtk_widget_get_style_context(btn_no);
    gtk_style_context_add_provider(context,
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_add_class(context, "no-button");

    // เพิ่มปุ่มเข้า dialog
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), btn_yes, GTK_RESPONSE_YES);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), btn_no, GTK_RESPONSE_NO);

    // ตั้ง default เป็นปุ่มยกเลิก
    gtk_widget_set_can_default(btn_no, TRUE);
    gtk_widget_grab_default(btn_no);

    // แสดง dialog และทุก widget
    gtk_widget_show_all(dialog);

    // รอผลตอบกลับ
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(message);
    g_object_unref(provider);

    if (response == GTK_RESPONSE_YES) {
        update_order_canceled(app, order_id, 1); // ยกเลิกออเดอร์
        refresh_data(app); // รีเฟรช listbox
    }
}

void on_btn_paid_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;

    int order_id = app->selected_order_id;
    gchar *message = g_strdup_printf("กรุณายืนยันการชำระเงินออเดอร์ #%d", order_id);

    // สร้าง dialog
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_QUESTION,
                                               GTK_BUTTONS_NONE,
                                               "%s", message);

    // โหลด CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style.css", NULL);

    // =========================
    // ปุ่ม Yes (ยืนยันจ่ายเงิน) สีเขียว ✔️
    // =========================
    GtkWidget *btn_yes = gtk_button_new();
    GtkWidget *hbox_yes = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *icon_yes = gtk_image_new_from_icon_name("emblem-ok", GTK_ICON_SIZE_BUTTON);
    GtkWidget *label_yes = gtk_label_new("ยืนยัน");

    gtk_box_pack_start(GTK_BOX(hbox_yes), icon_yes, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_yes), label_yes, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(btn_yes), hbox_yes);

    GtkStyleContext *context = gtk_widget_get_style_context(btn_yes);
    gtk_style_context_add_provider(context,
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_add_class(context, "yes-button");

    // =========================
    // ปุ่ม No (ยกเลิก) สีแดง ✖️
    // =========================
    GtkWidget *btn_no = gtk_button_new();
    GtkWidget *hbox_no = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *icon_no = gtk_image_new_from_icon_name("process-stop", GTK_ICON_SIZE_BUTTON);
    GtkWidget *label_no = gtk_label_new("ยกเลิก");

    gtk_box_pack_start(GTK_BOX(hbox_no), icon_no, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_no), label_no, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(btn_no), hbox_no);

    context = gtk_widget_get_style_context(btn_no);
    gtk_style_context_add_provider(context,
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_add_class(context, "no-button");

    // เพิ่มปุ่มเข้า dialog
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), btn_yes, GTK_RESPONSE_YES);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), btn_no, GTK_RESPONSE_NO);

    // ตั้ง default เป็นปุ่มยกเลิก
    gtk_widget_set_can_default(btn_no, TRUE);
    gtk_widget_grab_default(btn_no);

    // แสดง dialog และทุก widget
    gtk_widget_show_all(dialog);

    // รอผลตอบกลับ
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(message);
    g_object_unref(provider);

    if (response == GTK_RESPONSE_YES) {
        int status = 4; // จ่ายแล้ว
        update_order_status(app, order_id, status);

        refresh_data(app); // รีเฟรช listbox
        GtkListBoxRow *first_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), 0);
        if (first_row) {
            gtk_list_box_select_row(GTK_LIST_BOX(app->listbox), first_row);
            gtk_widget_grab_focus(GTK_WIDGET(first_row));
        }
    }
}

void btn_paid_clicked_cb(GtkButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;
    int order_id = app->selected_order_id;

    // สร้าง dialog เปล่า
    GtkWidget *dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "เลือกวิธีชำระเงิน");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app->window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    // เพิ่มข้อความด้านบน
    gchar *message = g_strdup_printf("กรุณาเลือกวิธีการชำระเงินของออเดอร์ #%d", order_id);
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *label = gtk_label_new(message);
    gtk_box_pack_start(GTK_BOX(content_area), label, TRUE, TRUE, 10);
    gtk_widget_show(label);
    g_free(message);

    // เพิ่มปุ่ม
    GtkWidget *btn_transfer = gtk_dialog_add_button(GTK_DIALOG(dialog), "โอนเงิน", 4);
    GtkWidget *btn_cash     = gtk_dialog_add_button(GTK_DIALOG(dialog), "เงินสด", 5);
    GtkWidget *btn_cancel   = gtk_dialog_add_button(GTK_DIALOG(dialog), "ยกเลิก", GTK_RESPONSE_CANCEL);

    // ตั้ง default เป็น Cancel
    gtk_widget_set_can_default(btn_cancel, TRUE);
    gtk_widget_grab_default(btn_cancel);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);

    // โหลด CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style.css", NULL);

    // กำหนด style ปุ่ม
    GtkStyleContext *ctx;
    ctx = gtk_widget_get_style_context(btn_transfer);
    gtk_style_context_add_provider(ctx, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_add_class(ctx, "transfer");

    ctx = gtk_widget_get_style_context(btn_cash);
    gtk_style_context_add_provider(ctx, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_add_class(ctx, "cash");

    ctx = gtk_widget_get_style_context(btn_cancel);
    gtk_style_context_add_provider(ctx, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_add_class(ctx, "cancel");

    // แสดง dialog
    gtk_widget_show_all(dialog);

    // รอผลตอบกลับ
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_object_unref(provider);

    if (result == 4) {
        update_order_status(app, order_id, 4); // โอนเงิน
    } else if (result == 5) {
        update_order_status(app, order_id, 5); // เงินสด
    }
    // Cancel ไม่ทำอะไร
}

void on_calendar_button_clicked(GtkButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("เลือกวันที่",
        GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_OK", GTK_RESPONSE_OK,
        "_Cancel", GTK_RESPONSE_CANCEL,
        NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *calendar = gtk_calendar_new();
    gtk_container_add(GTK_CONTAINER(content), calendar);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        guint year, month, day;
        gtk_calendar_get_date(GTK_CALENDAR(calendar), &year, &month, &day);

        // month ใน GtkCalendar เริ่มที่ 0 → ต้อง +1
        snprintf(app->filter_date, sizeof(app->filter_date),
                 "%04d-%02d-%02d", year, month + 1, day);

        gtk_label_set_text(GTK_LABEL(app->lbl_filter_date), app->filter_date);

        refresh_data(app);
    }

    gtk_widget_destroy(dialog);
}

// ฟังก์ชัน callback สำหรับอัปเดตวันที่
void calendar_day_selected_cb(GtkCalendar *calendar, gpointer user_data) {
    GtkWidget *label = GTK_WIDGET(user_data);
    guint year, month, day;
    gtk_calendar_get_date(calendar, &year, &month, &day); // month เริ่มจาก 0
    gchar *date_str = g_strdup_printf("%04u-%02u-%02u", year, month + 1, day);
    gtk_label_set_text(GTK_LABEL(label), date_str);
    g_free(date_str);
}

Order *get_order_by_id(const char *api_base_url, const char *machine_name, const char *token, int order_id) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    Order *order = malloc(sizeof(Order));
    if (!order) return NULL;
    memset(order, 0, sizeof(Order));

    struct MemoryStruct chunk = {0};  // ต้องมี struct สำหรับ WriteMemoryCallback

    // สร้าง URL แบบ query parameter
    char url[512];
    snprintf(url, sizeof(url), "%s/api/store/orders/%d?machine_name=%s&token=%s",
             api_base_url, order_id, machine_name, token);
    url[sizeof(url)-1] = '\0';

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.memory);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(order);
        return NULL;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    // --- parse JSON ---
    JsonParser *parser = json_parser_new();
    GError *error = NULL;
    if (!json_parser_load_from_data(parser, chunk.memory, -1, &error)) {
        fprintf(stderr, "JSON parse error: %s\n", error->message);
        g_error_free(error);
        free(chunk.memory);
        g_object_unref(parser);
        free(order);
        return NULL;
    }

    JsonNode *root = json_parser_get_root(parser);
    JsonObject *root_obj = json_node_get_object(root);
    gboolean success = json_object_get_boolean_member(root_obj, "success");
    if (!success) {
        fprintf(stderr, "API returned failure\n");
        g_object_unref(parser);
        free(chunk.memory);
        free(order);
        return NULL;
    }

    JsonObject *order_obj = json_object_get_object_member(root_obj, "order");
    if (!order_obj) {
        g_object_unref(parser);
        free(chunk.memory);
        free(order);
        return NULL;
    }

    // --- เติมข้อมูลลง struct Order ---
    order->order_id = json_object_get_int_member(order_obj, "id");
    const char *line_name = json_object_get_string_member(order_obj, "line_name");
    const char *place = json_object_get_string_member(order_obj, "place");
    const char *delivery_time = json_object_get_string_member(order_obj, "delivery_time");
    const char *created_at = json_object_get_string_member(order_obj, "created_at");
    int cancelled = json_object_get_int_member(order_obj, "cancelled");

    strncpy(order->line_name, line_name ? line_name : "", sizeof(order->line_name)-1);
    order->line_name[sizeof(order->line_name)-1] = '\0';
    strncpy(order->place, place ? place : "", sizeof(order->place)-1);
    order->place[sizeof(order->place)-1] = '\0';
    strncpy(order->delivery_time, delivery_time ? delivery_time : "", sizeof(order->delivery_time)-1);
    order->delivery_time[sizeof(order->delivery_time)-1] = '\0';
    strncpy(order->statusText, cancelled ? "ยกเลิก" : "รอส่ง", sizeof(order->statusText)-1);
    order->statusText[sizeof(order->statusText)-1] = '\0';
    strncpy(order->created_at, created_at ? created_at : "", sizeof(order->created_at)-1);
    order->created_at[sizeof(order->created_at)-1] = '\0';
    order->cancelled = cancelled;

    // --- items ---
    JsonArray *items_array = json_object_get_array_member(order_obj, "items");
    order->item_count = 0;
    if (items_array) {
        int n = json_array_get_length(items_array);
        for (int i=0; i<n && i<50; i++) {
            JsonObject *item_obj = json_array_get_object_element(items_array, i);
            const char *item_name = json_object_get_string_member(item_obj, "item_name");
            int qty = json_object_get_int_member(item_obj, "qty");
            const char *price_str = json_object_get_string_member(item_obj, "price");
            double price = price_str ? atof(price_str) : 0.0;

            const char *option_text = json_object_get_string_member(item_obj, "option_text");

            strncpy(order->items[i].name, item_name ? item_name : "", sizeof(order->items[i].name)-1);
            order->items[i].name[sizeof(order->items[i].name)-1] = '\0';

            strncpy(order->items[i].option_text, option_text ? option_text : "", sizeof(order->items[i].option_text)-1);
            order->items[i].option_text[sizeof(order->items[i].option_text)-1] = '\0';

            order->items[i].qty = qty;
            order->items[i].price = price;
            //g_print ("(%s)\n", order->items[i].option_text);
            order->item_count++;
        }
    }

    g_object_unref(parser);
    free(chunk.memory);

    return order;
}

// ตัวอย่าง callback
void on_radio_toggled(GtkToggleButton *button, gpointer user_data) {
    AppWidgets *app = (AppWidgets *)user_data;

    if (gtk_toggle_button_get_active(button)) {
        int monitor_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "monitor-id"));
        app->selected_monitor = monitor_id;
        //g_print("เลือก monitor = %d\n", monitor_id);
        refresh_data(app);
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    AppWidgets app;
    app.filter_date[0] = '\0'; // เริ่มต้นเป็น empty string
    load_dotenv_to_struct(&app, ".env");

    app.selected_index = -1;

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Kitchen Orders");
    gtk_window_maximize(GTK_WINDOW(app.window));

    gtk_widget_set_name(app.window, "main-window");

    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // HeaderBar
    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Orders");
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(app.window), header);

    app.btn_do = gtk_button_new_with_label("ทำรายการ");
    app.btn_done = gtk_button_new_with_label("สำเร็จ");
    gtk_widget_set_sensitive(app.btn_do, FALSE); // เริ่มต้น disable
    gtk_widget_set_sensitive(app.btn_done, FALSE); // เริ่มต้น disable
    
    app.btn_cancel = gtk_button_new_with_label("ยกเลิก");
    gtk_style_context_add_class(gtk_widget_get_style_context(app.btn_cancel), "destructive-action");
    
    // สมมติอยู่ใน main() หรือ function ที่สร้าง UI
    app.btn_paid = gtk_button_new_with_label("จ่ายแล้ว");

    // เพิ่ม style ถ้าต้องการ
    GtkStyleContext *context = gtk_widget_get_style_context(app.btn_paid);
    gtk_style_context_add_class(context, "suggested-action"); // หรือใช้ "destructive-action" ตามต้องการ

    GtkWidget *btn_up = gtk_button_new_from_icon_name("go-up-symbolic", GTK_ICON_SIZE_DIALOG);
    GtkWidget *btn_down = gtk_button_new_from_icon_name("go-down-symbolic", GTK_ICON_SIZE_DIALOG);

    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), app.btn_do);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), app.btn_done);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), btn_up);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), btn_down);

    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), app.btn_cancel);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), app.btn_paid);
    
    // ปุ่มเลือกวันที่
    GtkWidget *btn_calendar = gtk_button_new_from_icon_name("x-office-calendar", GTK_ICON_SIZE_BUTTON);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), btn_calendar);
    g_signal_connect(btn_calendar, "clicked", G_CALLBACK(on_calendar_button_clicked), &app);
    
    app.lbl_filter_date = gtk_label_new("ส่งวันนี้");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), app.lbl_filter_date);
    
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    GtkWidget *label = gtk_label_new("Monitor:");

    gtk_header_bar_pack_start (GTK_HEADER_BAR (header), sep);
    gtk_header_bar_pack_start (GTK_HEADER_BAR (header), label);

    // สร้าง radio buttons
    GtkWidget *rb1 = gtk_radio_button_new_with_label(NULL, "1");
    GtkWidget *rb2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb1), "2");
    GtkWidget *rb3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb1), "3");

    gtk_header_bar_pack_start (GTK_HEADER_BAR (header), rb1);
    gtk_header_bar_pack_start (GTK_HEADER_BAR (header), rb2);
    gtk_header_bar_pack_start (GTK_HEADER_BAR (header), rb3);

   
    // กำหนด monitor-id ให้ปุ่ม (ต้องใช้ GINT_TO_POINTER)
    g_object_set_data(G_OBJECT(rb1), "monitor-id", GINT_TO_POINTER(1));
    g_object_set_data(G_OBJECT(rb2), "monitor-id", GINT_TO_POINTER(2));
    g_object_set_data(G_OBJECT(rb3), "monitor-id", GINT_TO_POINTER(3));

    //printf("rb1=%p rb2=%p rb3=%p selected=%d\n", rb1, rb2, rb3, app.selected_monitor);

    //g_signal_handlers_block_by_func(rb1, G_CALLBACK(on_radio_toggled), &app);
    //g_signal_handlers_block_by_func(rb2, G_CALLBACK(on_radio_toggled), &app);
    //g_signal_handlers_block_by_func(rb3, G_CALLBACK(on_radio_toggled), &app);
    if (app.selected_monitor == 1) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb1), TRUE);
    } else if (app.selected_monitor == 2) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb2), TRUE);
    } else if (app.selected_monitor == 3) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb3), TRUE);
    }
    //g_signal_handlers_unblock_by_func(rb2, G_CALLBACK(on_radio_toggled), &app);

    // สามารถเชื่อม callback ได้ตามต้องการ
    g_signal_connect(rb1, "toggled", G_CALLBACK(on_radio_toggled), &app);
    g_signal_connect(rb2, "toggled", G_CALLBACK(on_radio_toggled), &app);
    g_signal_connect(rb3, "toggled", G_CALLBACK(on_radio_toggled), &app);
    
    app.clock_label = gtk_label_new("");
    gtk_widget_set_name(app.clock_label, "clock-label");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), app.clock_label);
    //update_clock(&app);
    //g_timeout_add_seconds(1, update_clock, &app);

    // ScrolledWindow + ListBox
    app.scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(app.window), app.scrolled);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(app.scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    app.listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(app.scrolled), app.listbox);

    // CSS
GtkCssProvider *css_provider = gtk_css_provider_new();
GError *css_error = NULL;
    
const char *css_data;
char buffer[1024];


GtkSettings *settings = gtk_settings_get_default();
gchar *theme_name = NULL;
g_object_get(settings, "gtk-theme-name", &theme_name, NULL);

gboolean dark_header = FALSE;
if (theme_name && g_str_has_suffix(theme_name, "-dark")) {
    dark_header = TRUE;
}

const char *clock_color = dark_header ? "#FFFFFF" : "#000000";
g_print ("clock color: %s\n", clock_color);
start_clock_thread(GTK_LABEL(app.clock_label));


snprintf(buffer, sizeof(buffer),
    "window, scrolledwindow, viewport, list box { background-color: #000000; }"
    "list row { background-color: #000000; }"
    "list row label { color: #666666; font-size: 48px; padding: 10px; }"
    "list row:selected { background-color: #333333; }"
    "list row:selected label { color: #ffffff; font-size: 48px; }"
    "#clock-label { color: %s; font-size: 64px; font-weight: bold; }",
    clock_color);

css_data = buffer;

gtk_css_provider_load_from_data(css_provider, css_data, -1, &css_error);

    if(css_error) {
        g_printerr("CSS error: %s\n", css_error->message);
        g_error_free(css_error);
    }
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_signal_connect(app.btn_do, "clicked", G_CALLBACK(btn_do_clicked_cb), &app);
    g_signal_connect(app.btn_done, "clicked", G_CALLBACK(btn_done_clicked_cb), &app);
    g_signal_connect(btn_up, "clicked", G_CALLBACK(scroll_listbox_up_cb), &app);
    g_signal_connect(btn_down, "clicked", G_CALLBACK(scroll_listbox_down_cb), &app);
    //g_signal_connect(app.btn_paid, "clicked", G_CALLBACK(on_btn_paid_clicked), &app);
    g_signal_connect(app.btn_paid, "clicked", G_CALLBACK(btn_paid_clicked_cb), &app);
    g_signal_connect(app.btn_cancel, "clicked", G_CALLBACK(btn_cancel_clicked_cb), &app);
    g_signal_connect(app.listbox, "row-selected", G_CALLBACK(on_row_selected), &app);

    refresh_data(&app);
    g_timeout_add(5000, refresh_data_timeout, &app);

    gtk_widget_show_all(app.window);
    
    setup_auto_screen_fade(GTK_WINDOW(app.window), 3600000);


    gtk_main();

    return 0;
}
