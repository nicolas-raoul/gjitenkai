#ifndef RADICAL_LIST_H
#define RADICAL_LIST_H

#include <gtk/gtk.h>

#include "kanjidic.h"

//radical per row in the radical list window
#define RADICAL_PER_ROW 18

GList *button_list;

/**
   declaration of a callback funcion defined in callbacks.c 
 */
extern void on_radical_button_clicked(GtkButton *button, kanjidic *kanjidic);

/**
   Create the list of radical window
 */
void radical_list_init(kanjidic *kanjidic);

void radical_list_update_sensitivity(kanjidic *kanjidic);

#endif
