#ifndef CLOCK_H
#define CLOCK_H

#include <gtk/gtk.h>

typedef struct {
    GtkLabel *clock_label;
    gboolean running;
} ClockData;

void start_clock_thread(GtkLabel *label);
void stop_clock_thread();

#endif
