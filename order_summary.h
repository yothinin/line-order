#ifndef ORDER_SUMMARY_H
#define ORDER_SUMMARY_H

#include <gtk/gtk.h>
#include "listbox.h"

// ฟังก์ชันสำหรับปุ่มสรุปยอด
void on_summary_clicked(GtkWidget *widget, AppWidgets *app);
void show_order_summary(GtkWidget *widget, gpointer user_data);
// ฟังก์ชันพิมพ์
void print_to_printer(const char *text);

#endif
