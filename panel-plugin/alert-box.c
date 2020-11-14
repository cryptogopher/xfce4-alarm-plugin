/*
 *  Copyright (C) 2020 cryptogopher
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4panel/xfce-panel-plugin.h>
#include <canberra.h>

#include "alert-box.h"


static gboolean playback_finished(gpointer button);
static void ca_playback_finished(ca_context *c, uint32_t id, int error_code, void *button);


// Utilities
static void
select_sound(GtkWidget *dialog, gboolean select)
{
  GtkBuilder *builder;
  GObject *object;
  const gchar *widgets[] = {"sound-play-box", "sound-loop-box", NULL};
  const gchar **widget;

  builder = g_object_get_data(G_OBJECT(dialog), "builder");

  if (!select)
  {
    object = gtk_builder_get_object(builder, "sound-chooser");
    g_return_if_fail(GTK_IS_FILE_CHOOSER_BUTTON(object));
    gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(object));
  }

  for (widget = widgets; *widget; widget++)
  {
    object = gtk_builder_get_object(builder, *widget);
    g_return_if_fail(GTK_IS_WIDGET(object));
    gtk_widget_set_sensitive(GTK_WIDGET(object), select);
  }
}

static void
play_sound(GtkWidget *dialog, gboolean play)
{
  GtkBuilder *builder;
  GObject *object, *image;
  GtkToggleButton *button;
  const gchar *widgets[] = {"sound-chooser", "clear-sound", "sound-loop-box", NULL};
  const gchar **widget;
  ca_context *context;
  ca_proplist *proplist;
  gchar *filename;
  int is_playing;

  builder = g_object_get_data(G_OBJECT(dialog), "builder");

  for (widget = widgets; *widget; widget++)
  {
    object = gtk_builder_get_object(builder, *widget);
    g_return_if_fail(GTK_IS_WIDGET(object));
    gtk_widget_set_sensitive(GTK_WIDGET(object), !play);
  }

  image = gtk_builder_get_object(builder, play ? "image-stop" : "image-play");
  g_return_if_fail(GTK_IS_IMAGE(image));
  object = gtk_builder_get_object(builder, "play-sound");
  g_return_if_fail(GTK_IS_BUTTON(object));
  button = GTK_TOGGLE_BUTTON(object);
  gtk_button_set_image(GTK_BUTTON(button), GTK_WIDGET(image));
  gtk_button_set_label(GTK_BUTTON(button), play ? "Stop" : "Play now");
  gtk_toggle_button_set_active(button, play);

  object = gtk_builder_get_object(builder, "alert-box");
  context = g_object_get_data(object, "ca-context");
  if (play)
  {
    object = gtk_builder_get_object(builder, "sound-chooser");
    g_return_if_fail(GTK_IS_FILE_CHOOSER(object));
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(object));

    g_warn_if_fail(!ca_proplist_create(&proplist));
    g_warn_if_fail(!ca_proplist_sets(proplist, CA_PROP_MEDIA_FILENAME, filename));
    g_warn_if_fail(!ca_context_play_full(context, 1, proplist, ca_playback_finished,
                                         button));
    g_warn_if_fail(!ca_proplist_destroy(proplist));

    g_free(filename);
  }
  else
  {
    g_warn_if_fail(!ca_context_playing(context, 1, &is_playing));
    if (is_playing)
      g_warn_if_fail(!ca_context_cancel(context, 1));
  }
}

static void
ca_playback_finished(ca_context *c, uint32_t id, int error_code, void *button)
{
  if (error_code != CA_ERROR_CANCELED)
    g_idle_add(playback_finished, button);
}

static gint
app_order_func(gconstpointer left, gconstpointer right)
{
  // TODO: make sorting case insensitive
  return g_utf8_collate(g_app_info_get_display_name(G_APP_INFO(left)),
                        g_app_info_get_display_name(G_APP_INFO(right)));
}


// Callbacks
static void
sound_chooser_file_set(GtkFileChooserButton *button, gpointer user_data)
{
  g_return_if_fail(GTK_IS_FILE_CHOOSER_BUTTON(button));

  select_sound(gtk_widget_get_toplevel(GTK_WIDGET(button)), TRUE);
}

static void
clear_sound_clicked(GtkButton *button, gpointer user_data)
{
  g_return_if_fail(GTK_IS_BUTTON(button));

  select_sound(gtk_widget_get_toplevel(GTK_WIDGET(button)), FALSE);
}

static void
play_sound_toggled(GtkToggleButton *button, gpointer user_data)
{
  g_return_if_fail(GTK_IS_TOGGLE_BUTTON(button));

  play_sound(gtk_widget_get_toplevel(GTK_WIDGET(button)),
             gtk_toggle_button_get_active(button));
}

static gboolean
playback_finished(gpointer button)
{
  g_return_val_if_fail(GTK_IS_TOGGLE_BUTTON(button), G_SOURCE_REMOVE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);

  return G_SOURCE_REMOVE;
}

static gboolean
program_separator_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  gboolean separator;
  gtk_tree_model_get(model, iter, 3, &separator, -1);
  return separator;
}

// External interface
void
init_alert_box(GtkBuilder *builder, const gchar *container_id)
{
  GObject *object, *source, *target;
  GtkWidget *alert_box;
  ca_context *context;
  GList *apps, *app_iter;
  GAppInfo *app;

  object = gtk_builder_get_object(builder, "alert-box");
  g_return_if_fail(GTK_IS_GRID(object));
  alert_box = GTK_WIDGET(object);

  // Create libcanberra context for playing sounds
  ca_context_create(&context);
  g_object_set_data(object, "ca-context", context);
  g_object_weak_ref(object, (GWeakNotify) G_CALLBACK(ca_context_destroy), context);

  // Only add symbols here. They will be connected by container.
  gtk_builder_add_callback_symbols(builder,
      "sound_chooser_file_set", G_CALLBACK(sound_chooser_file_set),
      "clear_sound_clicked", G_CALLBACK(clear_sound_clicked),
      "play_sound_toggled", G_CALLBACK(play_sound_toggled),
      NULL);

  // Connect alert box to container
  object = gtk_builder_get_object(builder, container_id);
  g_return_if_fail(GTK_IS_CONTAINER(object));
  g_object_ref(alert_box);
  gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(alert_box)), alert_box);
  gtk_container_add(GTK_CONTAINER(object), alert_box);
  g_object_unref(alert_box);

  // Fill program list:
  object = gtk_builder_get_object(builder, "program-store");
  g_return_if_fail(GTK_IS_LIST_STORE(object));
  // - (none)
  gtk_list_store_insert_with_values(GTK_LIST_STORE(object), NULL, -1,
                                    0, NULL,
                                    1, NULL,
                                    2, "(None)",
                                    3, FALSE, -1);
  // - separator
  gtk_list_store_insert_with_values(GTK_LIST_STORE(object), NULL, -1,
                                    0, NULL,
                                    1, NULL,
                                    2, NULL,
                                    3, TRUE, -1);
  // - file chooser
  gtk_list_store_insert_with_values(GTK_LIST_STORE(object), NULL, -1,
                                    0, NULL,
                                    1, NULL,
                                    2, "Choose program file...",
                                    3, FALSE, -1);
  // - separator
  gtk_list_store_insert_with_values(GTK_LIST_STORE(object), NULL, -1,
                                    0, NULL,
                                    1, NULL,
                                    2, NULL,
                                    3, TRUE, -1);
  // - applications
  apps = g_app_info_get_all();
  apps = g_list_sort(apps, app_order_func);
  app_iter = apps;
  while (app_iter)
  {
    app = app_iter->data;
    if (g_app_info_should_show(app))
      gtk_list_store_insert_with_values(GTK_LIST_STORE(object), NULL, -1,
                                        0, app,
                                        1, g_app_info_get_icon(app),
                                        2, g_app_info_get_display_name(app),
                                        3, FALSE, -1);
    app_iter = app_iter->next;
  }
  g_list_free_full(apps, g_object_unref);

  object = gtk_builder_get_object(builder, "program");
  g_return_if_fail(GTK_IS_COMBO_BOX(object));
  gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(object), program_separator_func, NULL,
                                       NULL);
  gtk_combo_box_set_active(GTK_COMBO_BOX(object), 0);

  source = gtk_builder_get_object(builder, "limit-loops");
  g_return_if_fail(GTK_IS_SWITCH(source));
  target = gtk_builder_get_object(builder, "loop-count");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(target));
  g_object_bind_property(source, "active", target, "sensitive", G_BINDING_SYNC_CREATE);

  source = gtk_builder_get_object(builder, "limit-runtime");
  g_return_if_fail(GTK_IS_SWITCH(source));
  target = gtk_builder_get_object(builder, "limit-period-box");
  g_return_if_fail(GTK_IS_BOX(target));
  g_object_bind_property(source, "active", target, "sensitive", G_BINDING_SYNC_CREATE);

  source = gtk_builder_get_object(builder, "repeat");
  g_return_if_fail(GTK_IS_SWITCH(source));
  target = gtk_builder_get_object(builder, "repeats-interval-box");
  g_return_if_fail(GTK_IS_BOX(target));
  g_object_bind_property(source, "active", target, "sensitive", G_BINDING_SYNC_CREATE);
  target = gtk_builder_get_object(builder, "limit-repeats-box");
  g_return_if_fail(GTK_IS_BOX(target));
  g_object_bind_property(source, "active", target, "sensitive", G_BINDING_SYNC_CREATE);

  source = gtk_builder_get_object(builder, "limit-repeats");
  g_return_if_fail(GTK_IS_SWITCH(source));
  target = gtk_builder_get_object(builder, "repeat-count");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(target));
  g_object_bind_property(source, "active", target, "sensitive", G_BINDING_SYNC_CREATE);
}
