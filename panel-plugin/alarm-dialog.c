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

#include <libxfce4panel/xfce-panel-plugin.h>

#include "alarm-dialog.h"
#include "alarm-dialog_ui.h"


static void
alarm_to_dialog(Alarm *alarm, GtkBuilder *builder)
{
}

static void
alarm_from_dialog(Alarm *alarm, GtkBuilder *builder)
{
  GObject *object;
  gint hours, minutes, seconds;
  gint value;

  g_return_if_fail(alarm != NULL);
  g_return_if_fail(GTK_IS_BUILDER(builder));

  object = gtk_builder_get_object(builder, "name");
  g_return_if_fail(GTK_IS_ENTRY(object));
  g_free(alarm->name);
  alarm->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(object)));

  object = gtk_builder_get_object(builder, "hours");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(object));
  hours = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(object));
  object = gtk_builder_get_object(builder, "minutes");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(object));
  minutes = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(object));
  object = gtk_builder_get_object(builder, "seconds");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(object));
  seconds = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(object));
  g_clear_pointer(&alarm->time, g_date_time_unref);
  alarm->time = g_date_time_new_local(1, 1, 1, hours, minutes, seconds);

  object = gtk_builder_get_object(builder, "progress");
  g_return_if_fail(GTK_IS_SWITCH(object));
  alarm->show_progress = gtk_switch_get_active(GTK_SWITCH(object));
  if (alarm->show_progress)
  {
    object = gtk_builder_get_object(builder, "color");
    g_return_if_fail(GTK_IS_COLOR_BUTTON(object));
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(object), &alarm->color);
  }

  object = gtk_builder_get_object(builder, "recurrence");
  g_return_if_fail(GTK_IS_STACK(object));
  gtk_container_child_get(GTK_CONTAINER(object),
                          gtk_stack_get_visible_child(GTK_STACK(object)),
                          "position", &value,
                          NULL);
  g_return_if_fail(value >= 0 && value < TYPE_COUNT);
  alarm->type = value;
}

void
show_alarm_dialog(GtkWidget *parent, XfcePanelPlugin *panel_plugin, Alarm **alarm)
{
  GtkBuilder *builder;
  GObject *dialog;

  g_return_if_fail(GTK_IS_WINDOW(parent));
  g_return_if_fail(XFCE_IS_PANEL_PLUGIN(panel_plugin));
  g_return_if_fail(alarm != NULL);

  builder = alarm_builder_new(panel_plugin, alarm_dialog_ui, alarm_dialog_ui_length);
  g_return_if_fail(GTK_IS_BUILDER(builder));

  dialog = gtk_builder_get_object(builder, "alarm-dialog");
  g_return_if_fail(GTK_IS_DIALOG(dialog));
  xfce_panel_plugin_take_window(panel_plugin, GTK_WINDOW(dialog));
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));

  //gtk_builder_connect_signals(builder, NULL);

  if (*alarm)
    alarm_to_dialog(*alarm, builder);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_APPLY)
  {
    if (*alarm == NULL)
      *alarm = g_slice_new0(Alarm);

    alarm_from_dialog(*alarm, builder);
  }

  gtk_widget_destroy(GTK_WIDGET(dialog));
  g_object_unref(builder);
}
