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

#include "properties-dialog.h"
#include "properties-dialog_ui.h"
#include "alarm-dialog.h"


// Utilities
static void
alarm_to_tree_iter(Alarm *alarm, GtkListStore *store, GtkTreeIter *iter)
{
  gchar *time, *color;

  g_return_if_fail(alarm != NULL);
  g_return_if_fail(GTK_IS_LIST_STORE(store));
  g_return_if_fail(iter != NULL);

  time = g_strdup_printf("<span size=\"large\" weight=\"normal\">%02u:%02u</span>" \
                         "<span size=\"small\" weight=\"normal\">:%02u</span>",
                         alarm->h, alarm->m, alarm->s);
  /* Setting color through markup preserves proper color on item selection (as
   * opposed to setting it through cell renderer background property). */
  color = g_strdup_printf("<span size=\"x-large\" foreground=\"%s\">\xe2\x96\x8a</span>",
                          alarm->color);
  gtk_list_store_set(store, iter,
                     COL_ICON_NAME, alarm_type_icons[alarm->type],
                     COL_TIME, time,
                     COL_COLOR, color,
                     COL_NAME, alarm->name,
                     -1);
  g_free(time);
  g_free(color);
}


// Callbacks
static void
new_alarm(GtkToolButton *add_button, AlarmPlugin *plugin)
{
  GtkWidget *parent;
  Alarm *alarm = NULL;
  GtkBuilder *builder;
  GObject *tree_view;
  GtkTreeModel *store;
  GtkTreeIter tree_iter;

  g_return_if_fail(GTK_IS_TOOL_BUTTON(add_button));
  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));

  parent = gtk_widget_get_toplevel(GTK_WIDGET(add_button));
  show_alarm_dialog(parent, XFCE_PANEL_PLUGIN(plugin), &alarm);
  if (alarm)
    plugin->alarms = g_list_append(plugin->alarms, alarm);
  else
    return;

  save_alarm(plugin, alarm);

  builder = g_object_get_data(G_OBJECT(parent), "builder");
  tree_view = gtk_builder_get_object(builder, "alarm-list");
  g_return_if_fail(GTK_IS_TREE_VIEW(tree_view));
  store = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
  gtk_list_store_append(GTK_LIST_STORE(store), &tree_iter);
  alarm_to_tree_iter(alarm, GTK_LIST_STORE(store), &tree_iter);
}

static void
edit_alarm(GtkToolButton *edit_button, AlarmPlugin *plugin)
{
}

static void
remove_alarm(GtkToolButton *remove_button, AlarmPlugin *plugin)
{
}


// External interface
void
show_properties_dialog(XfcePanelPlugin *panel_plugin)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);
  GtkBuilder *builder;
  GObject *dialog, *object;
  GtkListStore *store;
  GList *alarm_iter;
  GtkTreeIter tree_iter;

  builder = alarm_builder_new(panel_plugin, properties_dialog_ui,
                              properties_dialog_ui_length);
  g_return_if_fail(GTK_IS_BUILDER(builder));

  dialog = gtk_builder_get_object(builder, "properties-dialog");
  g_return_if_fail(GTK_IS_DIALOG(dialog));
  g_object_set_data_full(dialog, "builder", builder, g_object_unref);
  xfce_panel_plugin_take_window(panel_plugin, GTK_WINDOW(dialog));

  xfce_panel_plugin_block_menu(panel_plugin);
  g_object_weak_ref(dialog, (GWeakNotify) G_CALLBACK(xfce_panel_plugin_unblock_menu),
                    panel_plugin);

  /* NOTE: once type list consists only of static types it can be moved to
   * header file below column names for clarity */
  store = gtk_list_store_new(COL_COUNT,
                             G_TYPE_STRING,
                             G_TYPE_STRING,
                             G_TYPE_STRING,
                             G_TYPE_STRING);
  object = gtk_builder_get_object(builder, "alarm-list");
  g_return_if_fail(GTK_IS_TREE_VIEW(object));
  gtk_tree_view_set_model(GTK_TREE_VIEW(object), GTK_TREE_MODEL(store));
  alarm_iter = plugin->alarms;
  while (alarm_iter)
  {
    gtk_list_store_append(store, &tree_iter);
    alarm_to_tree_iter(alarm_iter->data, store, &tree_iter);
    alarm_iter = alarm_iter->next;
  }
  g_object_unref(store);

  gtk_builder_add_callback_symbols(builder,
                                   "new_alarm", G_CALLBACK(new_alarm),
                                   "edit_alarm", G_CALLBACK(edit_alarm),
                                   "remove_alarm", G_CALLBACK(remove_alarm),
                                   NULL);
  gtk_builder_connect_signals(builder, plugin);

  gtk_widget_show(GTK_WIDGET(dialog));
}
