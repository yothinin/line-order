#ifndef SCREENFADE_H
#define SCREENFADE_H

#include <gtk/gtk.h>

// เรียกเมื่ออยากให้ fade full screen 2 วินาที
void start_screen_fade(GtkWindow *parent);

// setup auto fade ทุก interval_ms (มิลลิวินาที)
void setup_auto_screen_fade(GtkWindow *window, guint interval_ms);

#endif // SCREENFADE_H
