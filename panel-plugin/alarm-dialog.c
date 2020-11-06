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

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif

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
  GtkTreeModel *tree_model;
  GtkTreeIter tree_iter;
  GtkTreePath *tree_path;
  guint value;

  /* Code relies on sensivity/activity defaults set in .glade. Whenever defaults
   * are changed in .glade, code has to be updated accordingly. */

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

  if (alarm->color[0] == '\0')
  {
    object = gtk_builder_get_object(builder, "progress");
    g_return_if_fail(GTK_IS_SWITCH(object));
    gtk_switch_set_active(GTK_SWITCH(object), FALSE);
  }
  else if (gdk_rgba_parse(&color, alarm->color))
  {
    object = gtk_builder_get_object(builder, "color");
    g_return_if_fail(GTK_IS_COLOR_BUTTON(object));
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(object), &color);
  }

  object = gtk_builder_get_object(builder, "autostart");
  g_return_if_fail(GTK_IS_SWITCH(object));
  gtk_switch_set_active(GTK_SWITCH(object), alarm->autostart);

  object = gtk_builder_get_object(builder, "autostop");
  g_return_if_fail(GTK_IS_SWITCH(object));
  gtk_switch_set_active(GTK_SWITCH(object), alarm->autostop);

  object = gtk_builder_get_object(builder, "autostart-on-resume");
  g_return_if_fail(GTK_IS_SWITCH(object));
  gtk_switch_set_active(GTK_SWITCH(object), alarm->autostart_on_resume);

  object = gtk_builder_get_object(builder, "autostop-on-suspend");
  g_return_if_fail(GTK_IS_SWITCH(object));
  gtk_switch_set_active(GTK_SWITCH(object), alarm->autostop_on_suspend);

  object = gtk_builder_get_object(builder, "recurrence");
  g_return_if_fail(GTK_IS_STACK(object));
  objects = gtk_container_get_children(GTK_CONTAINER(object));
  gtk_stack_set_visible_child(GTK_STACK(object), g_list_nth_data(objects, alarm->type));
  g_list_free(objects);

  if (alarm->triggered_timer != NULL)
  {
    object = gtk_builder_get_object(builder, "trigger-timer");
    g_return_if_fail(GTK_IS_SWITCH(object));
    gtk_switch_set_active(GTK_SWITCH(object), TRUE);

    object = gtk_builder_get_object(builder, "timer-combo");
    g_return_if_fail(GTK_IS_COMBO_BOX(object));
    g_return_if_fail(gtk_combo_box_set_active_id(GTK_COMBO_BOX(object),
                                                 alarm->triggered_timer->uuid));
  }

  if (alarm->rerun.every != NO_RERUN)
  {
    object = gtk_builder_get_object(builder, "rerun-clock");
    g_return_if_fail(GTK_IS_SWITCH(object));
    gtk_switch_set_active(GTK_SWITCH(object), TRUE);

    if (alarm->rerun.every >= RERUN_DOW)
    {
      object = gtk_builder_get_object(builder, "rerun-dow");
      g_return_if_fail(GTK_IS_RADIO_BUTTON(object));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(object), TRUE);

      object = gtk_builder_get_object(builder, "dow-store");
      g_return_if_fail(GTK_IS_TREE_MODEL(object));
      tree_model = GTK_TREE_MODEL(object);

      object = gtk_builder_get_object(builder, "dow-view");
      g_return_if_fail(GTK_IS_ICON_VIEW(object));
      if (gtk_tree_model_get_iter_first(tree_model, &tree_iter))
        do
        {
          gtk_tree_model_get(tree_model, &tree_iter, COL_DATA, &value, -1);
          if (alarm->rerun.every & (1 << value))
          {
            tree_path = gtk_tree_model_get_path(tree_model, &tree_iter);
            gtk_icon_view_select_path(GTK_ICON_VIEW(object), tree_path);
            gtk_tree_path_free(tree_path);
          }
        }
        while (gtk_tree_model_iter_next(tree_model, &tree_iter));
    }
    else
    {
      object = gtk_builder_get_object(builder, "rerun-ndays");
      g_return_if_fail(GTK_IS_RADIO_BUTTON(object));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(object), TRUE);

      object = gtk_builder_get_object(builder, "rerun-multiplier");
      g_return_if_fail(GTK_IS_SPIN_BUTTON(object));
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(object), -alarm->rerun.every);

      object = gtk_builder_get_object(builder, "rerun-mode");
      g_return_if_fail(GTK_IS_COMBO_BOX_TEXT(object));
      gtk_combo_box_set_active(GTK_COMBO_BOX(object), alarm->rerun.mode);
    }
  }
}

// TODO: return boolean, false if error
static void
alarm_from_dialog(Alarm *alarm, GtkBuilder *builder)
{
  GObject *object;
  guint value;
  GdkRGBA color;
  GtkTreeModel *tree_model;
  GtkTreeIter tree_iter;
  GList *items, *item_iter;

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
  alarm->color[0] = '\0';
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

  object = gtk_builder_get_object(builder, "autostart");
  g_return_if_fail(GTK_IS_SWITCH(object));
  alarm->autostart = gtk_switch_get_active(GTK_SWITCH(object));

  object = gtk_builder_get_object(builder, "autostop");
  g_return_if_fail(GTK_IS_SWITCH(object));
  alarm->autostop = gtk_switch_get_active(GTK_SWITCH(object));

  object = gtk_builder_get_object(builder, "autostart-on-resume");
  g_return_if_fail(GTK_IS_SWITCH(object));
  alarm->autostart_on_resume = gtk_switch_get_active(GTK_SWITCH(object));

  object = gtk_builder_get_object(builder, "autostop-on-suspend");
  g_return_if_fail(GTK_IS_SWITCH(object));
  alarm->autostop_on_suspend = gtk_switch_get_active(GTK_SWITCH(object));

  object = gtk_builder_get_object(builder, "recurrence");
  g_return_if_fail(GTK_IS_STACK(object));
  gtk_container_child_get(GTK_CONTAINER(object),
                          gtk_stack_get_visible_child(GTK_STACK(object)),
                          "position", &value,
                          NULL);
  g_return_if_fail(value < TYPE_COUNT);
  alarm->type = value;

  object = gtk_builder_get_object(builder, "trigger-timer");
  g_return_if_fail(GTK_IS_SWITCH(object));
  alarm->triggered_timer = NULL;
  if (gtk_switch_get_active(GTK_SWITCH(object)))
  {
    object = gtk_builder_get_object(builder, "timer-combo");
    g_return_if_fail(GTK_IS_COMBO_BOX(object));
    g_return_if_fail(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(object), &tree_iter));
    gtk_tree_model_get(gtk_combo_box_get_model(GTK_COMBO_BOX(object)), &tree_iter,
                       COL_DATA, &alarm->triggered_timer, -1);
    if (alarm->triggered_timer == NULL)
      alarm->triggered_timer = alarm;
  }

  object = gtk_builder_get_object(builder, "rerun-clock");
  g_return_if_fail(GTK_IS_SWITCH(object));
  alarm->rerun.every = NO_RERUN;
  if (gtk_switch_get_active(GTK_SWITCH(object)))
  {
    object = gtk_builder_get_object(builder, "rerun-dow");
    g_return_if_fail(GTK_IS_RADIO_BUTTON(object));
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object)))
    {
      object = gtk_builder_get_object(builder, "dow-view");
      g_return_if_fail(GTK_IS_ICON_VIEW(object));
      items = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(object));

      object = gtk_builder_get_object(builder, "dow-store");
      g_return_if_fail(GTK_IS_TREE_MODEL(object));
      tree_model = GTK_TREE_MODEL(object);

      item_iter = items;
      while (item_iter)
      {
        if (gtk_tree_model_get_iter(tree_model, &tree_iter, (GtkTreePath*) item_iter->data))
        {
          gtk_tree_model_get(tree_model, &tree_iter, COL_DATA, &value, -1);
          alarm->rerun.every |= (1 << value);
        }
        item_iter = item_iter->next;
      }
      g_return_if_fail(alarm->rerun.every >= RERUN_DOW &&
                       alarm->rerun.every <= RERUN_EVERYDAY);
      g_list_free_full(items, (GDestroyNotify) gtk_tree_path_free);
    }
    else
    {
      object = gtk_builder_get_object(builder, "rerun-ndays");
      g_return_if_fail(GTK_IS_RADIO_BUTTON(object));
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object)))
      {
        object = gtk_builder_get_object(builder, "rerun-multiplier");
        g_return_if_fail(GTK_IS_SPIN_BUTTON(object));
        alarm->rerun.every = - gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(object));
        g_return_if_fail(alarm->rerun.every < RERUN_DOW);

        object = gtk_builder_get_object(builder, "rerun-mode");
        g_return_if_fail(GTK_IS_COMBO_BOX_TEXT(object));
        alarm->rerun.mode = gtk_combo_box_get_active(GTK_COMBO_BOX(object));
        g_return_if_fail(alarm->rerun.mode >= 0 && alarm->rerun.mode < RERUN_MODE_COUNT);
      }
    }
  }
}

static gboolean
is_sensitive_and_active(GBinding *binding, const GValue *from_value, GValue *to_value,
                        gpointer user_data)
{
  GObject *source = g_binding_get_source(binding);

  g_return_val_if_fail(GTK_IS_RADIO_BUTTON(source), FALSE);
  g_return_val_if_fail(G_VALUE_HOLDS_BOOLEAN(to_value), FALSE);
  g_value_set_boolean(to_value, gtk_widget_get_sensitive(GTK_WIDGET(source)) &&
                                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(source)));
  return TRUE;
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
  Alarm *triggered_timer;

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
  gtk_list_store_insert_with_values(GTK_LIST_STORE(object), NULL, -1,
                                    0, *alarm, 1, "self", 2, "", -1);
  gtk_combo_box_set_active(GTK_COMBO_BOX(target), 0);
  alarm_iter = plugin->alarms;
  while (alarm_iter)
  {
    triggered_timer = alarm_iter->data;
    if (triggered_timer->type == TYPE_TIMER && triggered_timer != *alarm)
      gtk_list_store_insert_with_values(GTK_LIST_STORE(object), NULL, -1,
                                        0, triggered_timer,
                                        1, triggered_timer->name,
                                        2, triggered_timer->uuid, -1);
    alarm_iter = alarm_iter->next;
  }

  source = gtk_builder_get_object(builder, "rerun-clock");
  g_return_if_fail(GTK_IS_SWITCH(source));
  target = gtk_builder_get_object(builder, "rerun-dow");
  g_return_if_fail(GTK_IS_RADIO_BUTTON(target));
  g_object_bind_property(source, "active", target, "sensitive", G_BINDING_SYNC_CREATE);
  target = gtk_builder_get_object(builder, "rerun-ndays");
  g_return_if_fail(GTK_IS_RADIO_BUTTON(target));
  g_object_bind_property(source, "active", target, "sensitive", G_BINDING_SYNC_CREATE);

  source = gtk_builder_get_object(builder, "rerun-dow");
  target = gtk_builder_get_object(builder, "dow-view");
  g_return_if_fail(GTK_IS_ICON_VIEW(target));
  g_object_bind_property_full(source, "active", target, "sensitive",
                              G_BINDING_SYNC_CREATE,
                              is_sensitive_and_active, NULL, NULL, NULL);
  g_object_bind_property_full(source, "sensitive", target, "sensitive",
                              G_BINDING_SYNC_CREATE,
                              is_sensitive_and_active, NULL, NULL, NULL);

  source = gtk_builder_get_object(builder, "rerun-ndays");
  target = gtk_builder_get_object(builder, "rerun-multiplier");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(target));
  g_object_bind_property_full(source, "active", target, "sensitive",
                              G_BINDING_SYNC_CREATE,
                              is_sensitive_and_active, NULL, NULL, NULL);
  g_object_bind_property_full(source, "sensitive", target, "sensitive",
                              G_BINDING_SYNC_CREATE,
                              is_sensitive_and_active, NULL, NULL, NULL);
  target = gtk_builder_get_object(builder, "rerun-mode");
  g_return_if_fail(GTK_IS_COMBO_BOX_TEXT(target));
  g_object_bind_property_full(source, "active", target, "sensitive",
                              G_BINDING_SYNC_CREATE,
                              is_sensitive_and_active, NULL, NULL, NULL);
  g_object_bind_property_full(source, "sensitive", target, "sensitive",
                              G_BINDING_SYNC_CREATE,
                              is_sensitive_and_active, NULL, NULL, NULL);

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
