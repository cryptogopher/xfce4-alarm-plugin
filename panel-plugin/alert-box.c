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

#include "alert-box.h"

// Callbacks
static void
sound_chooser_file_set(GtkFileChooserButton *button, gpointer user_data)
{
  GtkBuilder *builder;
  GObject *object;

  g_return_if_fail(GTK_IS_FILE_CHOOSER_BUTTON(button));

  object = G_OBJECT(gtk_widget_get_toplevel(GTK_WIDGET(button)));
  builder = g_object_get_data(object, "builder");

  object = gtk_builder_get_object(builder, "sound-play-box");
  g_return_if_fail(GTK_IS_BOX(object));
  gtk_widget_set_sensitive(GTK_WIDGET(object), TRUE);

  object = gtk_builder_get_object(builder, "sound-loop-box");
  g_return_if_fail(GTK_IS_BOX(object));
  gtk_widget_set_sensitive(GTK_WIDGET(object), TRUE);
}


// External interface
void
init_alert_box(GtkBuilder *builder, const gchar *container_id)
{
  GObject *object, *source, *target;
  GtkWidget *alert_box;

  // Connect alert box to container
  object = gtk_builder_get_object(builder, "alert-box");
  g_return_if_fail(GTK_IS_GRID(object));
  alert_box = GTK_WIDGET(object);

  // Only add symbols here. They will be connected by container.
  gtk_builder_add_callback_symbols(builder,
      "sound_chooser_file_set", G_CALLBACK(sound_chooser_file_set),
      NULL);

  object = gtk_builder_get_object(builder, container_id);
  g_return_if_fail(GTK_IS_CONTAINER(object));
  g_object_ref(alert_box);
  gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(alert_box)), alert_box);
  gtk_container_add(GTK_CONTAINER(object), alert_box);
  g_object_unref(alert_box);

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
