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
#include <xfconf/xfconf.h>

#include "properties-dialog.h"
#include "properties-dialog_ui.h"
#include "alarm-dialog.h"
#include "alert.h"

#define UNICODE_BLOCK "\xe2\x96\x8a"

// Utilities
static void
alarm_to_tree_iter(Alarm *alarm, GtkListStore *store, GtkTreeIter *iter)
{
  gchar *time, *color = NULL;

  g_return_if_fail(alarm != NULL);
  g_return_if_fail(GTK_IS_LIST_STORE(store));
  g_return_if_fail(iter != NULL);

  time = g_strdup_printf("<span size=\"large\" weight=\"normal\">%02u:%02u</span>" \
                         "<span size=\"small\" weight=\"normal\">:%02u</span>",
                         alarm->time/3600, alarm->time%3600/60, alarm->time%60);
  /* Setting color through markup preserves proper color on item selection (as
   * opposed to setting it through cell renderer background property). */
  if (alarm->color[0] != '\0')
    color = g_strdup_printf("<span size=\"x-large\" foreground=\"%s\">" UNICODE_BLOCK
                            "</span>", alarm->color);

  gtk_list_store_set(store, iter,
                     COL_DATA, alarm,
                     COL_ICON_NAME, alarm_type_icons[alarm->type],
                     COL_TIME, time,
                     COL_COLOR, color,
                     COL_NAME, alarm->name,
                     -1);

  g_free(time);
  g_free(color);
}

static Alarm*
get_selected_alarm(GtkBuilder *builder, GtkTreeModel **model, GtkTreeIter *iter)
{
  GObject *selection;
  Alarm *alarm = NULL;

  selection = gtk_builder_get_object(builder, "alarm-selection");
  g_return_val_if_fail(GTK_IS_TREE_SELECTION(selection), NULL);
  if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection), model, iter))
    gtk_tree_model_get(*model, iter, COL_DATA, &alarm, -1);

  return alarm;
}

static void
new_alarm(AlarmPlugin *plugin, GtkWidget *dialog)
{
  Alarm *alarm = NULL;
  GtkBuilder *builder;
  GObject *store;
  GtkTreeIter tree_iter;

  show_alarm_dialog(dialog, XFCE_PANEL_PLUGIN(plugin), &alarm);
  if (alarm)
    plugin->alarms = g_list_append(plugin->alarms, alarm);
  else
    return;
  save_alarm_settings(plugin, alarm);

  builder = g_object_get_data(G_OBJECT(dialog), "builder");
  store = gtk_builder_get_object(builder, "alarm-store");
  g_return_if_fail(GTK_IS_LIST_STORE(store));
  gtk_list_store_append(GTK_LIST_STORE(store), &tree_iter);
  alarm_to_tree_iter(alarm, GTK_LIST_STORE(store), &tree_iter);
}

static void
edit_alarm(AlarmPlugin *plugin, GtkWidget *dialog)
{
  GtkBuilder *builder;
  Alarm *alarm;
  GtkTreeModel *store;
  GtkTreeIter tree_iter;

  builder = g_object_get_data(G_OBJECT(dialog), "builder");

  alarm = get_selected_alarm(builder, &store, &tree_iter);
  if (alarm == NULL)
    return;
  show_alarm_dialog(dialog, XFCE_PANEL_PLUGIN(plugin), &alarm);
  save_alarm_settings(plugin, alarm);

  alarm_to_tree_iter(alarm, GTK_LIST_STORE(store), &tree_iter);
}

static void
remove_alarm(AlarmPlugin *plugin, GtkWidget *dialog)
{
  GtkBuilder *builder;
  Alarm *alarm;
  GList *alarm_iter, *next_iter;
  GtkTreeModel *store;
  GtkTreeIter tree_iter;

  builder = g_object_get_data(G_OBJECT(dialog), "builder");
  alarm = get_selected_alarm(builder, &store, &tree_iter);
  if (alarm == NULL)
    return;

  gtk_list_store_remove(GTK_LIST_STORE(store), &tree_iter);

  reset_alarm_settings(plugin, alarm);

  alarm_iter = g_list_find(plugin->alarms, alarm);
  next_iter = alarm_iter->next;
  plugin->alarms = g_list_delete_link(plugin->alarms, alarm_iter);
  save_alarm_positions(plugin, next_iter, NULL);

  g_clear_pointer(&alarm, alarm_free);
}


// Callbacks
static void
new_button_clicked(GtkToolButton *button, AlarmPlugin *plugin)
{
  g_return_if_fail(GTK_IS_TOOL_BUTTON(button));
  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));

  new_alarm(plugin, gtk_widget_get_toplevel(GTK_WIDGET(button)));
}

static void
edit_button_clicked(GtkToolButton *button, AlarmPlugin *plugin)
{
  g_return_if_fail(GTK_IS_TOOL_BUTTON(button));
  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));

  edit_alarm(plugin, gtk_widget_get_toplevel(GTK_WIDGET(button)));
}

static void
remove_button_clicked(GtkToolButton *button, AlarmPlugin *plugin)
{
  g_return_if_fail(GTK_IS_TOOL_BUTTON(button));
  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));

  remove_alarm(plugin, gtk_widget_get_toplevel(GTK_WIDGET(button)));
}

static gboolean
alarm_view_button_press(GtkWidget *widget, GdkEvent *event, AlarmPlugin *plugin)
{
  g_return_val_if_fail(GTK_IS_TREE_VIEW(widget), FALSE);
  g_return_val_if_fail(event->button.window ==
                       gtk_tree_view_get_bin_window(GTK_TREE_VIEW(widget)), FALSE);
  g_return_val_if_fail(XFCE_IS_ALARM_PLUGIN(plugin), FALSE);

  if ((event->type == GDK_DOUBLE_BUTTON_PRESS) &&
      gtk_tree_view_is_blank_at_pos(GTK_TREE_VIEW(widget), event->button.x, event->button.y,
                                    NULL, NULL, NULL, NULL))
    new_alarm(plugin, gtk_widget_get_toplevel(GTK_WIDGET(widget)));

  return FALSE;
}

static void
alarm_view_row_activated(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column,
                         AlarmPlugin *plugin)
{
  g_return_if_fail(GTK_IS_TREE_VIEW(view));
  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));

  edit_alarm(plugin, gtk_widget_get_toplevel(GTK_WIDGET(view)));
}

static void
alarm_selection_changed(GtkTreeSelection *selection, GtkWidget *dialog)
{
  // Dialog required as param, because selection is not a GtkWidget
  gboolean selected;
  GtkBuilder *builder;

  g_return_if_fail(GTK_IS_TREE_SELECTION(selection));
  g_return_if_fail(GTK_IS_DIALOG(dialog));

  selected = gtk_tree_selection_get_selected(GTK_TREE_SELECTION(selection), NULL, NULL);
  builder = g_object_get_data(G_OBJECT(dialog), "builder");
  g_return_if_fail(GTK_IS_BUILDER(builder));

  set_sensitive(builder, selected, "edit", "remove", NULL);
}

static void
alarm_store_row_changed(GtkTreeModel *store, GtkTreePath *path, GtkTreeIter *iter,
                        AlarmPlugin *plugin)
{
  Alarm *alarm = NULL;
  gint list_position;
  gint store_position;
  GList *iter_from, *iter_to;

  g_return_if_fail(GTK_IS_LIST_STORE(store));
  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));
  // "row-changed" signal is emitted before DND source is removed from GtkListStore
  g_return_if_fail((gtk_tree_model_iter_n_children(store, NULL) - 1) ==
                   (gint) g_list_length(plugin->alarms));

  /* After DND reordering of store item, positions of some Alarms on plugin->alarms
   * list will be different than positions in store. plugin->alarms has to be reordered
   * accordingly and updated positions saved. */
  gtk_tree_model_get(store, iter, COL_DATA, &alarm, -1);
  g_return_if_fail(alarm != NULL);
  list_position = g_list_index(plugin->alarms, alarm);
  g_return_if_fail(list_position != -1);
  store_position = gtk_tree_path_get_indices(path)[0];
  // Correct store_position for the not-yet-removed DND source if required
  if (list_position < store_position)
    store_position--;

  if (list_position == store_position)
    return;

  plugin->alarms = g_list_remove(plugin->alarms, alarm);
  iter_to = g_list_nth(plugin->alarms, MAX(list_position, store_position));
  plugin->alarms = g_list_insert(plugin->alarms, alarm, store_position);
  iter_from = g_list_nth(plugin->alarms, MIN(list_position, store_position));
  save_alarm_positions(plugin, iter_from, iter_to);
}

static void
alarm_store_row_inserted(GtkTreeModel *store, GtkTreePath *path, GtkTreeIter *iter,
                         GtkWidget *view)
{
  g_return_if_fail(GTK_IS_LIST_STORE(store));
  g_return_if_fail(GTK_IS_TREE_VIEW(view));

  gtk_tree_view_set_cursor(GTK_TREE_VIEW(view), path, NULL, FALSE);
}


// External interface
// TODO: return GtkDialog which will be destroyed by caller if necessary
// (otherwise return NULL)
void
show_properties_dialog(XfcePanelPlugin *panel_plugin)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);
  GtkBuilder *builder;
  GObject *dialog, *object;
  GList *alarm_iter;
  GtkTreeIter tree_iter;

  builder = alarm_builder_new(panel_plugin, "properties-dialog", &dialog,
                              properties_dialog_ui, properties_dialog_ui_length,
                              NULL);
  g_return_if_fail(GTK_IS_BUILDER(builder));
  g_return_if_fail(GTK_IS_DIALOG(dialog));

  object = gtk_builder_get_object(builder, "alert-frame");
  g_return_if_fail(GTK_IS_CONTAINER(object));
  g_return_if_fail(show_alert_box(plugin->alert, panel_plugin, GTK_CONTAINER(object)));

  xfce_panel_plugin_take_window(panel_plugin, GTK_WINDOW(dialog));

  xfce_panel_plugin_block_menu(panel_plugin);
  g_object_weak_ref(dialog, (GWeakNotify) G_CALLBACK(xfce_panel_plugin_unblock_menu),
                    panel_plugin);

  object = gtk_builder_get_object(builder, "alarm-store");
  g_return_if_fail(GTK_IS_LIST_STORE(object));
  alarm_iter = plugin->alarms;
  while (alarm_iter)
  {
    gtk_list_store_append(GTK_LIST_STORE(object), &tree_iter);
    alarm_to_tree_iter(alarm_iter->data, GTK_LIST_STORE(object), &tree_iter);
    alarm_iter = alarm_iter->next;
  }

  gtk_builder_add_callback_symbols(builder,
      "new_button_clicked", G_CALLBACK(new_button_clicked),
      "edit_button_clicked", G_CALLBACK(edit_button_clicked),
      "remove_button_clicked", G_CALLBACK(remove_button_clicked),
      "alarm_view_button_press", G_CALLBACK(alarm_view_button_press),
      "alarm_view_row_activated", G_CALLBACK(alarm_view_row_activated),
      "alarm_selection_changed", G_CALLBACK(alarm_selection_changed),
      "alarm_store_row_changed", G_CALLBACK(alarm_store_row_changed),
      "alarm_store_row_inserted", G_CALLBACK(alarm_store_row_inserted),
      NULL);
  gtk_builder_connect_signals(builder, plugin);

  gtk_widget_show(GTK_WIDGET(dialog));
}
