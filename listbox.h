#ifndef LISTBOX_H
#define LISTBOX_H

#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include <curl/curl.h>

#include <qrencode.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define PAPER_WIDTH_MM 80
#define DPI 203                   // thermal printer ส่วนมาก ~203 dpi
#define PAPER_WIDTH_PX ((PAPER_WIDTH_MM/25.4)*DPI)

typedef enum {
    MEASURE_ONLY,
    DRAW_TEXT
} DrawMode;

typedef struct {
    char name[128];
    int qty;
    double price;
    char option_text[128]; // เพิ่มช่องสำหรับเก็บ option
} OrderItem;


typedef struct {
    int order_id;
    char line_name[128];
    char place[128];
    char delivery_time[32];
    char statusText[128];
    char created_at[32];
    int cancelled;
    int item_count;
    OrderItem items[50];
} Order;



typedef struct {
    char *data;
    size_t size;
} MemoryStruct;

struct MemoryStruct {
    char *memory;
    size_t size;
};


typedef struct {
    GtkWidget *window;
    GtkWidget *listbox;
    GtkWidget *scrolled;
    GtkWidget *btn_do;
    GtkWidget *btn_done;
    GtkWidget *btn_cancel;
    GtkWidget *clock_label;
    GtkWidget *btn_paid;
    gint selected_index;
    gint selected_order_id;
    gint selected_monitor;

    char machine_name[128];
    char token[128];
    char api_base_url[256];   // 🔹 เพิ่มตรงนี้
    
    char filter_date[11];       // YYYY-MM-DD
    
    GtkWidget *btn_calendar;
    GtkWidget *lbl_filter_date;
    GtkWidget *header_bar; // ถ้ายังไม่มี HeaderBar

} AppWidgets;

typedef struct {
    AppWidgets *app;
    int order_id;
    GtkWidget *spinner_dialog;
} CancelTaskData;


// ===================== Forward Declarations =====================
size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
gchar* fetch_orders_json(const char *url);
void select_first_row(AppWidgets *app);

void update_order_status(AppWidgets *app, gint order_id, gint status);
void update_order_canceled(AppWidgets *app, int order_id, int canceled);

void populate_listbox(AppWidgets *app, const gchar *json_data);
void refresh_data(AppWidgets *app);

void refocus_selected_row(AppWidgets *app);
void scroll_listbox_to_row(AppWidgets *app, gint idx);

void btn_do_clicked_cb(GtkButton *button, gpointer user_data);
void btn_done_clicked_cb(GtkButton *button, gpointer user_data);
void btn_cancel_clicked_cb(GtkButton *button, gpointer user_data);

void on_row_selected(GtkListBox *box, GtkListBoxRow *row, AppWidgets *app);

gboolean refresh_data_timeout(gpointer user_data);
gboolean update_clock(gpointer user_data);

void scroll_listbox_up_cb(GtkButton *button, gpointer user_data);
void scroll_listbox_down_cb(GtkButton *button, gpointer user_data);

void btn_cancel_clicked_cb(GtkButton *button, gpointer user_data);
void do_cancel_order(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
void on_cancel_done(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
void btn_paid_clicked_cb(GtkButton *button, gpointer user_data);
void print_slip_fullxxx(Order *order);
Order *get_order_by_id(const char *api_base_url, const char *machine_name, const char *token, int order_id);

#endif // LISTBOX_H
