// slip.h
#ifndef SLIP_H
#define SLIP_H

#include "listbox.h"
#include <cairo.h>
#include <pango/pangocairo.h>
#include <fcntl.h>
#include <unistd.h>

char *generate_promptpay(const char *mobile, double amount);
void draw_text(PangoLayout *layout, cairo_t *cr, int x, int *y, const char *text, int font_size, int bold);
void print_slip_full(AppWidgets *app, Order *order);

#endif
