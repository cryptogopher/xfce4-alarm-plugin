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
#include <xfconf/xfconf.h>

#include "common.h"
#include "alert.h"
#include "alarm-plugin.h"
#include "alarm.h"
#include "alarm-dialog.h"
#include "alarm-dialog_ui.h"
#include "alert-box.h"

enum DoWColumns
{
  DOW_COL_DATA,
  DOW_COL_NAME,
  DOW_COL_COUNT
};


// Utilities
static void
alarm_to_dialog(Alarm *alarm, GtkBuilder *builder)
{
  GObject *object;
  GList *objects;
  GtkTreeModel *tree_model;
  GtkTreeIter tree_iter;
  GtkTreePath *tree_path;
  guint value;
  gchar *str_value;

  /* Code relies on sensivity/activity defaults set in .glade. Whenever defaults
   * are changed in .glade, code has to be updated accordingly. */

  g_return_if_fail(alarm != NULL);
  g_return_if_fail(GTK_IS_BUILDER(builder));

  object = gtk_builder_get_object(builder, "name");
  g_return_if_fail(GTK_IS_ENTRY(object));
  gtk_entry_set_text(GTK_ENTRY(object), alarm->name);

  object = gtk_builder_get_object(builder, "time");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(object));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(object), alarm->time);

  object = gtk_builder_get_object(builder, "progress");
  g_return_if_fail(GTK_IS_SWITCH(object));
  gtk_switch_set_active(GTK_SWITCH(object), GPOINTER_TO_INT(alarm->color));
  if (alarm->color)
  {
    object = gtk_builder_get_object(builder, "color");
    g_return_if_fail(GTK_IS_COLOR_BUTTON(object));
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(object), alarm->color);
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

  object = gtk_builder_get_object(builder, "custom-alert");
  g_return_if_fail(GTK_IS_SWITCH(object));
  gtk_switch_set_active(GTK_SWITCH(object), alarm->alert != NULL);

  object = gtk_builder_get_object(builder, "recurrence");
  g_return_if_fail(GTK_IS_STACK(object));
  objects = gtk_container_get_children(GTK_CONTAINER(object));
  gtk_stack_set_visible_child(GTK_STACK(object), g_list_nth_data(objects, alarm->type));
  g_list_free(objects);

  if (alarm->triggered_timer != NULL)
  {
    object = gtk_builder_get_object(builder, "triggered-timer-combo");
    g_return_if_fail(GTK_IS_COMBO_BOX(object));
    str_value = g_strdup_printf("alarm-%u", alarm->triggered_timer->id);
    g_return_if_fail(gtk_combo_box_set_active_id(GTK_COMBO_BOX(object), str_value));
    g_clear_pointer(&str_value, g_free);
  }

  if (alarm->rerun_every != NO_RERUN)
  {
    object = gtk_builder_get_object(builder, "rerun-clock");
    g_return_if_fail(GTK_IS_SWITCH(object));
    gtk_switch_set_active(GTK_SWITCH(object), TRUE);

    if (alarm->rerun_every >= RERUN_DOW)
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
          gtk_tree_model_get(tree_model, &tree_iter, DOW_COL_DATA, &value, -1);
          if (alarm->rerun_every & (1 << value))
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
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(object), -alarm->rerun_every);

      object = gtk_builder_get_object(builder, "rerun-mode");
      g_return_if_fail(GTK_IS_COMBO_BOX_TEXT(object));
      gtk_combo_box_set_active(GTK_COMBO_BOX(object), alarm->rerun_mode);
    }
  }
}

static gboolean
alarm_from_dialog(Alarm *alarm, Alert *bound_alert, GtkBuilder *builder)
{
  GObject *object;
  guint value;
  gchar *alarm_strid;
  GdkRGBA color;
  GtkTreeModel *tree_model;
  GtkTreeIter tree_iter;
  GList *items, *item_iter;

  g_return_val_if_fail(alarm != NULL, FALSE);
  g_return_val_if_fail(GTK_IS_BUILDER(builder), FALSE);

  object = gtk_builder_get_object(builder, "name");
  g_return_val_if_fail(GTK_IS_ENTRY(object), FALSE);
  g_free(alarm->name);
  alarm->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(object)));

  object = gtk_builder_get_object(builder, "time");
  g_return_val_if_fail(GTK_IS_SPIN_BUTTON(object), FALSE);
  alarm->time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(object));

  object = gtk_builder_get_object(builder, "progress");
  g_return_val_if_fail(GTK_IS_SWITCH(object), FALSE);
  gdk_rgba_free(g_steal_pointer(&alarm->color));
  if (gtk_switch_get_active(GTK_SWITCH(object)))
  {
    object = gtk_builder_get_object(builder, "color");
    g_return_val_if_fail(GTK_IS_COLOR_BUTTON(object), FALSE);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(object), &color);
    alarm->color = gdk_rgba_copy(&color);
  }

  object = gtk_builder_get_object(builder, "autostart");
  g_return_val_if_fail(GTK_IS_SWITCH(object), FALSE);
  alarm->autostart = gtk_switch_get_active(GTK_SWITCH(object));

  object = gtk_builder_get_object(builder, "autostop");
  g_return_val_if_fail(GTK_IS_SWITCH(object), FALSE);
  alarm->autostop = gtk_switch_get_active(GTK_SWITCH(object));

  object = gtk_builder_get_object(builder, "autostart-on-resume");
  g_return_val_if_fail(GTK_IS_SWITCH(object), FALSE);
  alarm->autostart_on_resume = gtk_switch_get_active(GTK_SWITCH(object));

  object = gtk_builder_get_object(builder, "autostop-on-suspend");
  g_return_val_if_fail(GTK_IS_SWITCH(object), FALSE);
  alarm->autostop_on_suspend = gtk_switch_get_active(GTK_SWITCH(object));

  object = gtk_builder_get_object(builder, "recurrence");
  g_return_val_if_fail(GTK_IS_STACK(object), FALSE);
  gtk_container_child_get(GTK_CONTAINER(object),
                          gtk_stack_get_visible_child(GTK_STACK(object)),
                          "position", &value,
                          NULL);
  g_return_val_if_fail(value < ALARM_TYPE_COUNT, FALSE);
  alarm->type = value;

  object = gtk_builder_get_object(builder, "triggered-timer-combo");
  g_return_val_if_fail(GTK_IS_COMBO_BOX(object), FALSE);
  g_return_val_if_fail(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(object), &tree_iter),
                       FALSE);
  gtk_tree_model_get(gtk_combo_box_get_model(GTK_COMBO_BOX(object)), &tree_iter,
                     TT_COL_DATA, &alarm->triggered_timer, TT_COL_ID, &alarm_strid, -1);
  if (alarm_strid == NULL)
    alarm->triggered_timer = alarm;

  object = gtk_builder_get_object(builder, "rerun-clock");
  g_return_val_if_fail(GTK_IS_SWITCH(object), FALSE);
  alarm->rerun_every = NO_RERUN;
  if (gtk_switch_get_active(GTK_SWITCH(object)))
  {
    object = gtk_builder_get_object(builder, "rerun-dow");
    g_return_val_if_fail(GTK_IS_RADIO_BUTTON(object), FALSE);
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object)))
    {
      object = gtk_builder_get_object(builder, "dow-view");
      g_return_val_if_fail(GTK_IS_ICON_VIEW(object), FALSE);
      items = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(object));

      object = gtk_builder_get_object(builder, "dow-store");
      g_return_val_if_fail(GTK_IS_TREE_MODEL(object), FALSE);
      tree_model = GTK_TREE_MODEL(object);

      item_iter = items;
      while (item_iter)
      {
        if (gtk_tree_model_get_iter(tree_model, &tree_iter, (GtkTreePath*) item_iter->data))
        {
          gtk_tree_model_get(tree_model, &tree_iter, DOW_COL_DATA, &value, -1);
          alarm->rerun_every |= (1 << value);
        }
        item_iter = item_iter->next;
      }
      g_list_free_full(items, (GDestroyNotify) gtk_tree_path_free);
      g_return_val_if_fail(alarm->rerun_every >= RERUN_DOW &&
                           alarm->rerun_every <= RERUN_EVERYDAY, FALSE);
    }
    else
    {
      object = gtk_builder_get_object(builder, "rerun-ndays");
      g_return_val_if_fail(GTK_IS_RADIO_BUTTON(object), FALSE);
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(object)))
      {
        object = gtk_builder_get_object(builder, "rerun-multiplier");
        g_return_val_if_fail(GTK_IS_SPIN_BUTTON(object), FALSE);
        alarm->rerun_every = - gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(object));
        g_return_val_if_fail(alarm->rerun_every < RERUN_DOW, FALSE);

        object = gtk_builder_get_object(builder, "rerun-mode");
        g_return_val_if_fail(GTK_IS_COMBO_BOX_TEXT(object), FALSE);
        alarm->rerun_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(object));
        g_return_val_if_fail(alarm->rerun_mode >= 0 &&
                             alarm->rerun_mode < RERUN_MODE_COUNT, FALSE);
      }
    }
  }

  object = gtk_builder_get_object(builder, "custom-alert");
  g_return_val_if_fail(GTK_IS_SWITCH(object), FALSE);
  if (gtk_switch_get_active(GTK_SWITCH(object)))
    if (alarm->alert == NULL)
      alarm->alert = g_object_ref(bound_alert);
    else
      g_object_copy(G_OBJECT(bound_alert), G_OBJECT(alarm->alert));
  else
    g_clear_object(&alarm->alert);

  return TRUE;
}


// Callbacks
static void
recurrence_visible_child_notify(GtkWidget *widget, GParamSpec *child_property,
                                GtkWidget *dialog)
{
  guint position;
  GtkBuilder *builder;
  GObject *time_spin;

  g_return_if_fail(GTK_IS_STACK(widget));
  g_return_if_fail(GTK_IS_DIALOG(dialog));

  gtk_container_child_get(GTK_CONTAINER(widget),
                          gtk_stack_get_visible_child(GTK_STACK(widget)),
                          "position", &position,
                          NULL);
  g_return_if_fail(position < ALARM_TYPE_COUNT);

  builder = g_object_get_data(G_OBJECT(dialog), "builder");
  g_return_if_fail(GTK_IS_BUILDER(builder));

  time_spin = gtk_builder_get_object(builder, "time");
  g_return_if_fail(GTK_IS_SPIN_BUTTON(time_spin));
  gtk_spin_button_set_range(GTK_SPIN_BUTTON(time_spin),
                            TIME_LIMITS[2*position], TIME_LIMITS[2*position+1]);
}


// External interface
// TODO: return GtkDialog which will be destroyed by caller (because it's not
// destroyed in case of error)
void
show_alarm_dialog(GtkWidget *parent, XfcePanelPlugin *panel_plugin, Alarm **alarm)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);
  GtkBuilder *builder;
  GObject *dialog, *object, *store;
  GList *alarm_iter;
  Alarm *triggered_timer, *shown_alarm = NULL;
  gchar *alarm_strid = NULL;

  g_return_if_fail(GTK_IS_WINDOW(parent));
  g_return_if_fail(XFCE_IS_PANEL_PLUGIN(panel_plugin));
  g_return_if_fail(alarm != NULL);

  builder = alarm_builder_new(panel_plugin, "alarm-dialog", &dialog,
                              alarm_dialog_ui, alarm_dialog_ui_length,
                              NULL);
  g_return_if_fail(GTK_IS_BUILDER(builder));
  g_return_if_fail(GTK_IS_DIALOG(dialog));

  xfce_panel_plugin_take_window(panel_plugin, GTK_WINDOW(dialog));
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));

  gtk_builder_add_callback_symbols(builder,
      "time_input", G_CALLBACK(time_spin_input),
      "time_output", G_CALLBACK(time_spin_output),
      "recurrence_visible_child_notify", G_CALLBACK(recurrence_visible_child_notify),
      NULL);
  gtk_builder_connect_signals(builder, plugin);

  object = gtk_builder_get_object(builder, "triggered-timer-combo");
  g_return_if_fail(GTK_IS_COMBO_BOX(object));
  store = gtk_builder_get_object(builder, "triggered-timer-store");
  g_return_if_fail(GTK_IS_LIST_STORE(store));
  gtk_list_store_insert_with_values(GTK_LIST_STORE(store), NULL, -1,
                                    TT_COL_DATA, NULL,
                                    TT_COL_NAME, "",
                                    TT_COL_ID, "", -1);
  if (*alarm)
    alarm_strid = g_strdup_printf("alarm-%u", (*alarm)->id);
  gtk_list_store_insert_with_values(GTK_LIST_STORE(store), NULL, -1,
                                    TT_COL_DATA, *alarm,
                                    TT_COL_NAME, "self",
                                    TT_COL_ID, alarm_strid, -1);
  g_free(alarm_strid);

  alarm_iter = plugin->alarms;
  while (alarm_iter)
  {
    triggered_timer = alarm_iter->data;
    if (triggered_timer->type == ALARM_TYPE_TIMER && triggered_timer != *alarm)
    {
      alarm_strid = g_strdup_printf("alarm-%u", triggered_timer->id);
      gtk_list_store_insert_with_values(GTK_LIST_STORE(store), NULL, -1,
                                        TT_COL_DATA, triggered_timer,
                                        TT_COL_NAME, triggered_timer->name,
                                        TT_COL_ID, alarm_strid, -1);
      g_free(alarm_strid);
    }
    alarm_iter = alarm_iter->next;
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(object), 0);

  shown_alarm = alarm_new(NULL);
  g_object_copy(G_OBJECT(*alarm), G_OBJECT(shown_alarm));
  shown_alarm->alert = alert_new(NULL);
  if (*alarm)
    g_object_copy(G_OBJECT((*alarm)->alert), G_OBJECT(shown_alarm->alert));

  g_object_weak_ref(dialog, (GWeakNotify) G_CALLBACK(g_object_unref), shown_alarm);

  // TODO: add Alarm<=>dialog bindings
  alarm_to_dialog(*alarm, builder);
  object = gtk_builder_get_object(builder, "alert-alignment");
  g_return_if_fail(GTK_IS_CONTAINER(object));
  g_return_if_fail(show_alert_box(shown_alarm->alert, panel_plugin, GTK_CONTAINER(object)));

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_APPLY)
  {
    if (*alarm == NULL)
      *alarm = g_object_ref(shown_alarm);
    else
    {
      g_object_copy(G_OBJECT(shown_alarm), G_OBJECT(*alarm));
      // FIXME: copy alert only if set
      g_object_copy(G_OBJECT(shown_alarm->alert), G_OBJECT((*alarm)->alert));
    }
  }

  g_object_unref(shown_alarm);
  gtk_widget_destroy(GTK_WIDGET(dialog));
}
