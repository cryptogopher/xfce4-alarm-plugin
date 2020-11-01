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


// Utilities
static void
alarm_to_dialog(Alarm *alarm, GtkBuilder *builder)
{
  GObject *object;
  GdkRGBA color;
  GList *objects;

  g_return_if_fail(alarm != NULL);
  g_return_if_fail(GTK_IS_BUILDER(builder));

  object = gtk_builder_get_object(builder, "name");
  g_return_if_fail(GTK_IS_ENTRY(object));
  gtk_entry_set_text(GTK_ENTRY(object), alarm->name);

  object = gtk_builder_get_object(builder, "hours");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(object));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(object), alarm->h);

  object = gtk_builder_get_object(builder, "minutes");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(object));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(object), alarm->m);

  object = gtk_builder_get_object(builder, "seconds");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(object));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(object), alarm->s);

  object = gtk_builder_get_object(builder, "progress");
  g_return_if_fail(GTK_IS_SWITCH(object));
  gtk_switch_set_active(GTK_SWITCH(object), alarm->color[0] != '\0');
  if (alarm->color[0] != '\0' && gdk_rgba_parse(&color, alarm->color))
  {
    object = gtk_builder_get_object(builder, "color");
    g_return_if_fail(GTK_IS_COLOR_BUTTON(object));
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(object), &color);
  }

  object = gtk_builder_get_object(builder, "recurrence");
  g_return_if_fail(GTK_IS_STACK(object));
  objects = gtk_container_get_children(GTK_CONTAINER(object));
  gtk_stack_set_visible_child(GTK_STACK(object), g_list_nth_data(objects, alarm->type));
  g_list_free(objects);
}

// TODO: return boolean, false if error
static void
alarm_from_dialog(Alarm *alarm, GtkBuilder *builder)
{
  GObject *object;
  gint value;
  GdkRGBA color;

  g_return_if_fail(alarm != NULL);
  g_return_if_fail(GTK_IS_BUILDER(builder));

  object = gtk_builder_get_object(builder, "name");
  g_return_if_fail(GTK_IS_ENTRY(object));
  g_free(alarm->name);
  alarm->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(object)));

  object = gtk_builder_get_object(builder, "hours");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(object));
  alarm->h = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(object));

  object = gtk_builder_get_object(builder, "minutes");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(object));
  alarm->m = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(object));

  object = gtk_builder_get_object(builder, "seconds");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(object));
  alarm->s = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(object));

  object = gtk_builder_get_object(builder, "progress");
  g_return_if_fail(GTK_IS_SWITCH(object));
  if (gtk_switch_get_active(GTK_SWITCH(object)))
  {
    object = gtk_builder_get_object(builder, "color");
    g_return_if_fail(GTK_IS_COLOR_BUTTON(object));
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(object), &color);
    g_snprintf(alarm->color, sizeof(alarm->color),  "#%02x%02x%02x",
               (uint) (0.5 + color.red*255),
               (uint) (0.5 + color.green*255),
               (uint) (0.5 + color.blue*255));
  }
  else
    alarm->color[0] = '\0';

  object = gtk_builder_get_object(builder, "recurrence");
  g_return_if_fail(GTK_IS_STACK(object));
  gtk_container_child_get(GTK_CONTAINER(object),
                          gtk_stack_get_visible_child(GTK_STACK(object)),
                          "position", &value,
                          NULL);
  g_return_if_fail(value >= 0 && value < TYPE_COUNT);
  alarm->type = value;
}

// Callbacks
static void
time_wrapped(GtkSpinButton *wrapped_spin, GtkSpinButton *higher_spin)
{
  g_return_if_fail(GTK_IS_SPIN_BUTTON(wrapped_spin));
  g_return_if_fail(GTK_IS_SPIN_BUTTON(higher_spin));

  switch (gtk_spin_button_get_value_as_int(wrapped_spin))
  {
    case 0:
      gtk_spin_button_spin(higher_spin, GTK_SPIN_STEP_FORWARD, 1);
      break;
    case 59:
      gtk_spin_button_spin(higher_spin, GTK_SPIN_STEP_BACKWARD, 1);
      break;
  }
}


// External interface
void
show_alarm_dialog(GtkWidget *parent, XfcePanelPlugin *panel_plugin, Alarm **alarm)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);
  GtkBuilder *builder;
  GObject *dialog;
  GObject *object, *source, *target;
  GList *alarm_iter;
  Alarm *triggered_alarm;

  g_return_if_fail(GTK_IS_WINDOW(parent));
  g_return_if_fail(XFCE_IS_PANEL_PLUGIN(panel_plugin));
  g_return_if_fail(alarm != NULL);

  builder = alarm_builder_new(panel_plugin, alarm_dialog_ui, alarm_dialog_ui_length);
  g_return_if_fail(GTK_IS_BUILDER(builder));

  dialog = gtk_builder_get_object(builder, "alarm-dialog");
  g_return_if_fail(GTK_IS_DIALOG(dialog));
  xfce_panel_plugin_take_window(panel_plugin, GTK_WINDOW(dialog));
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));

  gtk_builder_add_callback_symbols(builder,
                              "time_wrapped", G_CALLBACK(time_wrapped),
                              NULL);
  gtk_builder_connect_signals(builder, plugin);

  source = gtk_builder_get_object(builder, "progress");
  g_return_if_fail(GTK_IS_SWITCH(source));
  target = gtk_builder_get_object(builder, "color");
  g_return_if_fail(GTK_IS_COLOR_BUTTON(target));
  g_object_bind_property(source, "active", target, "sensitive", G_BINDING_SYNC_CREATE);

  source = gtk_builder_get_object(builder, "trigger-timer");
  g_return_if_fail(GTK_IS_SWITCH(source));
  target = gtk_builder_get_object(builder, "timer-combo");
  g_return_if_fail(GTK_IS_COMBO_BOX(target));
  g_object_bind_property(source, "active", target, "sensitive", G_BINDING_SYNC_CREATE);

  object = gtk_builder_get_object(builder, "timer-store");
  g_return_if_fail(GTK_IS_LIST_STORE(object));
  gtk_list_store_insert_with_values(GTK_LIST_STORE(object), NULL, -1, 0, 0L, 1, "self", -1);
  gtk_combo_box_set_active(GTK_COMBO_BOX(target), 0);
  alarm_iter = plugin->alarms;
  while (alarm_iter)
  {
    triggered_alarm = alarm_iter->data;
    if (triggered_alarm->type == TYPE_TIMER && triggered_alarm != *alarm)
      gtk_list_store_insert_with_values(GTK_LIST_STORE(object), NULL, -1,
                                        0, triggered_alarm, 1, triggered_alarm->name, -1);
    alarm_iter = alarm_iter->next;
  }

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
