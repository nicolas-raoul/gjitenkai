#include "worddic.h"

void worddic_init (worddic *worddic)
{
  GError *err = NULL;
  worddic->definitions = gtk_builder_new ();
  gtk_builder_add_from_file (worddic->definitions,
                             UI_DEFINITIONS_FILE_WORDDIC, &err);
  if (err != NULL) {
    g_printerr
      ("Error while loading worddic definitions file: %s\n",
       err->message);
    g_error_free (err);
    gtk_main_quit ();
  }
  gtk_builder_connect_signals (worddic->definitions, worddic);

  //init the configuration handler
  worddic->settings = conf_init_handler(SETTINGS_WORDDIC);

  //load configuration 
  worddic->conf = worddic_conf_load(worddic);

  //by default, no mmaped dictionary
  worddic->conf->mmaped_dicfile = NULL;
  
  //by default search everything
  worddic->match_criteria_jp = ANY_MATCH;
  worddic->match_criteria_lat = ANY_MATCH;
  
  init_search_menu(worddic);

  //highlight style of the result text buffer
  GtkTextBuffer*textbuffer_search_results = (GtkTextBuffer*)
    gtk_builder_get_object(worddic->definitions, 
                           "textbuffer_search_results");
  
  GtkTextTag *highlight = gtk_text_buffer_create_tag (textbuffer_search_results,
                                                      "results_highlight",
                                                      "background-rgba",
                                                      worddic->conf->results_highlight_color,
                                                      NULL);
  worddic->conf->highlight = highlight;

  //japanese definition
  GtkTextTag *jap_def = gtk_text_buffer_create_tag (textbuffer_search_results,
                                                    "japanese_definition",
                                                    "foreground-rgba",
                                                    worddic->conf->jap_def_color,
                                                    "font", 
                                                    worddic->conf->jap_def_font,
                                                    NULL);
  worddic->conf->jap_def = jap_def;
  
  //japanese reading  
  GtkTextTag *jap_reading = gtk_text_buffer_create_tag (textbuffer_search_results,
                                                        "japanese_reading",
                                                        "foreground-rgba",
                                                        worddic->conf->jap_reading_color,
                                                        "font", 
                                                        worddic->conf->jap_reading_font,
                                                        NULL);
  worddic->conf->jap_reading = jap_reading;

  //translations
  GtkTextTag *translation = gtk_text_buffer_create_tag (textbuffer_search_results,
                                                        "translation",
                                                        "foreground-rgba",
                                                        worddic->conf->translation_color,
                                                        "font", 
                                                        worddic->conf->translation_font,
                                                        NULL);
  worddic->conf->translation = translation;
  
  //default font for the search results
  const gchar *font_name= worddic->conf->resultsfont;
  PangoFontDescription *font_desc = pango_font_description_from_string(font_name);

  //get the textview
  GtkTextView *textview_search_results = 
    (GtkTextView*)gtk_builder_get_object(worddic->definitions,
                                         "search_results");

  //apply the newly selected font to the results textview
  gtk_widget_override_font(GTK_WIDGET(textview_search_results), font_desc);
  
  //init the verb de-inflection mechanism
  Verbinit();

  //Init the preference window's widgets
  init_prefs_window(worddic);

  //init cursors
  cursor_selection = gdk_cursor_new(GDK_ARROW);
  cursor_default = gdk_cursor_new(GDK_XTERM);
  
}

void init_search_menu(worddic *worddic)
{
  //get the search options
  gint match_criteria_jp = worddic->match_criteria_jp;
  gint match_criteria_lat = worddic->match_criteria_lat;
 
  GtkRadioMenuItem* radio_jp;
  GtkRadioMenuItem* radio_lat;
 
  switch(match_criteria_lat){
  case EXACT_MATCH:
    radio_lat = gtk_builder_get_object(worddic->definitions, "menuitem_search_whole_expression");
    break;
  case WORD_MATCH:
    radio_lat = gtk_builder_get_object(worddic->definitions, "menuitem_search_latin_word");
    break;
  case ANY_MATCH:
    radio_lat = gtk_builder_get_object(worddic->definitions, "menuitem_search_latin_any");
    break;
  }

  switch(match_criteria_jp){
  case EXACT_MATCH:
    radio_jp = gtk_builder_get_object(worddic->definitions, "menuitem_search_japanese_exact");
    break;
  case START_WITH_MATCH:
    radio_jp = gtk_builder_get_object(worddic->definitions, "menuitem_search_japanese_start");
    break;
  case END_WITH_MATCH:
    radio_jp = gtk_builder_get_object(worddic->definitions, "menuitem_search_japanese_end");
    break;
  case ANY_MATCH:
    radio_jp = gtk_builder_get_object(worddic->definitions, "menuitem_search_japanese_any");
    break;
  }
 
  gtk_check_menu_item_set_active(radio_jp, TRUE);
  gtk_check_menu_item_set_active(radio_lat, TRUE); 
}

void print_entry(GtkTextBuffer *textbuffer_search_results,
                 GtkTextTag *highlight,
                 GList *entries_highlight,
                 GList *entries,
                 worddic *worddic){
  //get the textview
  GtkTextView *textview_search_results = 
    (GtkTextView*)gtk_builder_get_object(worddic->definitions, "search_results");

  GList *l = NULL;                   //browse results
  for (l = entries; l != NULL; l = l->next){
    GjitenDicentry *entry = l->data;

    GList *d = NULL;
    GtkTextIter iter;

    //Japanese definition
    for(d = entry->jap_definition;
        d != NULL;
        d = d->next){

      gchar* definition = (gchar*)d->data;

      gtk_text_buffer_insert_at_cursor(textbuffer_search_results, 
                                       worddic->conf->jap_def_start,
                                       strlen(worddic->conf->jap_def_start));

      gtk_text_buffer_get_end_iter (textbuffer_search_results, &iter);
      gtk_text_buffer_insert_with_tags(textbuffer_search_results,
                                       &iter,
                                       definition,
                                       strlen(definition),
                                       worddic->conf->jap_def,
                                       NULL);
      
      gtk_text_buffer_insert_at_cursor(textbuffer_search_results, 
                                       worddic->conf->jap_def_end,
                                       strlen(worddic->conf->jap_def_end));
    }

    //reading
    if(entry->jap_reading){
      for(d = entry->jap_reading;
          d != NULL;
          d = d->next){

        gchar* definition = (gchar*)d->data;

        gtk_text_buffer_insert_at_cursor(textbuffer_search_results, 
                                         worddic->conf->jap_reading_start,
                                         strlen(worddic->conf->jap_reading_start));

        gtk_text_buffer_get_end_iter (textbuffer_search_results, &iter);
        gtk_text_buffer_insert_with_tags(textbuffer_search_results,
                                         &iter,
                                         definition,
                                         strlen(definition),
                                         worddic->conf->jap_reading,
                                         NULL);
      
        gtk_text_buffer_insert_at_cursor(textbuffer_search_results, 
                                         worddic->conf->jap_reading_end,
                                         strlen(worddic->conf->jap_reading_end));
      }
    }

    //gloss
    for(d = entry->gloss;
        d != NULL;
        d = d->next){

      gchar* definition = (gchar*)d->data;

      gtk_text_buffer_insert_at_cursor(textbuffer_search_results, 
                                       worddic->conf->translation_start,
                                       strlen(worddic->conf->translation_start));

      gtk_text_buffer_get_end_iter (textbuffer_search_results, &iter);
      gtk_text_buffer_insert_with_tags(textbuffer_search_results,
                                       &iter,
                                       definition,
                                       strlen(definition),
                                       worddic->conf->translation,
                                       NULL);
      
      gtk_text_buffer_insert_at_cursor(textbuffer_search_results, 
                                       worddic->conf->translation_end,
                                       strlen(worddic->conf->translation_end));
    }
        
    gtk_text_buffer_insert_at_cursor(textbuffer_search_results, 
                                     "\n",
                                     strlen("\n"));
    //highlight
    highlight_result(textbuffer_search_results,
                     highlight,
                     entries_highlight->data);
    
    entries_highlight = entries_highlight->next;
  }
}

void highlight_result(GtkTextBuffer *textbuffer_search_results,
		      GtkTextTag *highlight,
		      const gchar *text_to_highlight){
  gboolean has_iter;
  GtkTextIter iter, match_start, match_end;
  gtk_text_buffer_get_start_iter (textbuffer_search_results, &iter);
  
  do{
    //search where the result string is located in the result buffer
    has_iter = gtk_text_iter_forward_search (&iter,
                                             text_to_highlight,
                                             GTK_TEXT_SEARCH_VISIBLE_ONLY,
                                             &match_start,
                                             &match_end,
                                             NULL);
    if(has_iter){
      //highlight at this location
      gtk_text_buffer_apply_tag (textbuffer_search_results,
                                 highlight,
                                 &match_start, 
                                 &match_end);

      //next iteration starts at the end of this iteration
      iter = match_end;
    }

  }while(has_iter);
}
