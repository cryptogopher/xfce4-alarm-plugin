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
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>

#include "alarm.h"
#include "alarm-dialog.h"
#include "properties-dialog_ui.h"


struct _AlarmPluginClass
{
  XfcePanelPluginClass parent;
};

// Only store things that have lifetime of the plugin here
struct _AlarmPlugin
{
  XfcePanelPlugin parent;

  GSList *alarms;
  GtkWidget *panel_button;
};


static void alarm_free_func(gpointer data)
{
  Alarm *alarm = (Alarm*) data;
  g_clear_pointer(&alarm->time, g_date_time_unref);
  g_slice_free(Alarm, alarm);
}

const gchar *alarm_type_icons[ALARM_COUNT] =
{
  "alarm-timer",
  "alarm-clock"
};

// Don't change order - column numbers are used in .glade
enum AlarmColumns
{
  COL_ICON_NAME,
  COL_TIME,
  COL_NAME,
  COL_COLOR,
  COL_COUNT
};


GtkBuilder* alarm_builder_new(XfcePanelPlugin *panel_plugin,
                              const gchar* buffer, gsize buffer_length)
{
  GtkBuilder *builder;
  GError *error = NULL;

  g_return_val_if_fail(XFCE_IS_PANEL_PLUGIN(panel_plugin), NULL);

  /* Hack to make sure GtkBuilder knows about the XfceTitledDialog object
   * https://wiki.xfce.org/releng/4.8/roadmap/libxfce4ui
   * http://bugzilla.gnome.org/show_bug.cgi?id=588238 */
  if (xfce_titled_dialog_get_type() == 0) return NULL;

  builder = gtk_builder_new();
  if (!gtk_builder_add_from_string(builder, buffer, buffer_length, &error))
  {
    g_critical("Failed to construct the builder for plugin %s-%d: %s.",
               xfce_panel_plugin_get_name (panel_plugin),
               xfce_panel_plugin_get_unique_id (panel_plugin),
               error->message);
    g_error_free(error);
    g_object_unref(builder);
  }

  return builder;
}


// Properties dialog signal handlers
static void
new_alarm(GtkToolButton *add_button, AlarmPlugin *plugin)
{
  GtkWidget *parent;
  Alarm *alarm = NULL;

  g_return_if_fail(GTK_IS_TOOL_BUTTON(add_button));
  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));

  parent = gtk_widget_get_toplevel(GTK_WIDGET(add_button));
  show_alarm_dialog(parent, plugin, &alarm);
  if (alarm)
    plugin->alarms = g_slist_append(plugin->alarms, alarm);
  // TODO: add to tree view
}

static void
show_properties_dialog(XfcePanelPlugin *panel_plugin)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);
  GtkBuilder *builder;
  GObject *dialog, *object;
  GtkListStore *store;

  builder = alarm_builder_new(panel_plugin, properties_dialog_ui, properties_dialog_ui_length);
  g_return_if_fail(GTK_IS_BUILDER(builder));

  dialog = gtk_builder_get_object(builder, "properties-dialog");
  g_return_if_fail(GTK_IS_DIALOG(dialog));
  /* Callback double casting to avoid GCC warning -Wcast-function-type
   * https://gitlab.gnome.org/GNOME/gnome-terminal/-/issues/96 */
  g_object_weak_ref(dialog, (GWeakNotify) G_CALLBACK(g_object_unref), builder);
  xfce_panel_plugin_take_window(panel_plugin, GTK_WINDOW(dialog));

  xfce_panel_plugin_block_menu(panel_plugin);
  g_object_weak_ref(dialog, (GWeakNotify) G_CALLBACK(xfce_panel_plugin_unblock_menu),
                    panel_plugin);

  store = gtk_list_store_new(COL_COUNT,
                             G_TYPE_STRING,
                             G_TYPE_STRING,
                             G_TYPE_STRING,
                             GDK_TYPE_RGBA);
  object = gtk_builder_get_object(builder, "alarm-list");
  g_return_if_fail(GTK_IS_TREE_VIEW(object));
  gtk_tree_view_set_model(GTK_TREE_VIEW(object), GTK_TREE_MODEL(store));
  g_object_weak_ref(dialog, (GWeakNotify) G_CALLBACK(g_object_unref), store);

  gtk_builder_add_callback_symbol(builder, "new_alarm", G_CALLBACK(new_alarm));
  gtk_builder_connect_signals(builder, plugin);

  gtk_widget_show(GTK_WIDGET(dialog));
}


// Panel signal handlers
static gboolean
panel_size_changed(XfcePanelPlugin *panel_plugin, gint size)
{
  // NOTE: no panel_plugin type check

  size /= xfce_panel_plugin_get_nrows(panel_plugin);
  if (xfce_panel_plugin_get_orientation(panel_plugin) == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request(GTK_WIDGET(panel_plugin), -1, size);
  else
    gtk_widget_set_size_request(GTK_WIDGET(panel_plugin), size, -1);

  return TRUE;
}

static void
panel_orientation_changed(XfcePanelPlugin *panel_plugin, GtkOrientation orientation)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);
  GList *progress_bar_iter;
  GtkWidget *progress_bar, *box = gtk_bin_get_child(GTK_BIN(plugin->panel_button));

  gtk_orientable_set_orientation(GTK_ORIENTABLE(box), orientation);

  progress_bar_iter = gtk_container_get_children(GTK_CONTAINER(box));
  while (progress_bar_iter)
  {
    progress_bar = GTK_WIDGET(progress_bar_iter->data);
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      gtk_orientable_set_orientation(GTK_ORIENTABLE(progress_bar), GTK_ORIENTATION_VERTICAL);
      gtk_progress_bar_set_inverted(GTK_PROGRESS_BAR(progress_bar), TRUE);
    } else {
      gtk_orientable_set_orientation(GTK_ORIENTABLE(progress_bar), GTK_ORIENTATION_HORIZONTAL);
      gtk_progress_bar_set_inverted(GTK_PROGRESS_BAR(progress_bar), FALSE);
    }
    progress_bar_iter = progress_bar_iter->next;
  }
  g_list_free(g_steal_pointer(&progress_bar_iter));

  panel_size_changed(panel_plugin, xfce_panel_plugin_get_size(panel_plugin));
}

static void
panel_button_toggled(GtkWidget *panel_button, AlarmPlugin *plugin)
{
  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));
  g_return_if_fail(GTK_IS_TOGGLE_BUTTON(panel_button));

  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_button)))
    return;

  //xfce_panel_plugin_block_autohide(XFCE_PANEL_PLUGIN(plugin), TRUE);
}



XFCE_PANEL_DEFINE_PLUGIN(AlarmPlugin, alarm_plugin)

static void
plugin_construct(XfcePanelPlugin *panel_plugin)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
  xfce_panel_plugin_menu_show_configure(panel_plugin);
  xfce_panel_plugin_set_small(panel_plugin, TRUE);

  gtk_widget_show(plugin->panel_button);
}

static void
plugin_free_data(XfcePanelPlugin *panel_plugin)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);

  g_slist_free_full(g_steal_pointer(&plugin->alarms), alarm_free_func);
}


static void
alarm_plugin_class_init(AlarmPluginClass *klass)
{
  //GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  XfcePanelPluginClass *plugin_class = XFCE_PANEL_PLUGIN_CLASS(klass);

  //gobject_class->get_property = directory_menu_plugin_get_property;
  //gobject_class->set_property = directory_menu_plugin_set_property;

  plugin_class->construct = plugin_construct;
  plugin_class->free_data = plugin_free_data;
  plugin_class->configure_plugin = show_properties_dialog;
  plugin_class->size_changed = panel_size_changed;
  plugin_class->orientation_changed = panel_orientation_changed;
  //plugin_class->remote_event = plugin_remote_event;
}

static void
alarm_plugin_init(AlarmPlugin *plugin)
{
  XfcePanelPlugin *panel_plugin = XFCE_PANEL_PLUGIN(plugin);
  GtkWidget *box;

  plugin->alarms = NULL;

  // Panel toggle button
  plugin->panel_button = xfce_panel_create_toggle_button();
  gtk_container_add(GTK_CONTAINER(plugin), plugin->panel_button);
  xfce_panel_plugin_add_action_widget(XFCE_PANEL_PLUGIN(plugin), plugin->panel_button);
  gtk_widget_set_name(plugin->panel_button, "alarm-button");
  g_signal_connect(G_OBJECT(plugin->panel_button), "toggled",
                   G_CALLBACK(panel_button_toggled), plugin);

  // Container for progress bars
  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(plugin->panel_button), box);

  // Blank progress bar insted of icon
  // TODO: change to insert icon when no active progress bars
  gtk_box_pack_start(GTK_BOX(box), gtk_progress_bar_new(), TRUE, FALSE, 0);
  panel_orientation_changed(panel_plugin, xfce_panel_plugin_get_orientation(panel_plugin));

  gtk_widget_show_all(box);
}
