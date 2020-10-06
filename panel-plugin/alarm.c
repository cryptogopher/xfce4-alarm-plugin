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
//#include <gtk/gtk.h>
//#include <libxfce4util/libxfce4util.h>
//#include <libxfce4panel/libxfce4panel.h>

#include "alarm.h"
#include "alarm_ui.h"


struct _AlarmPluginClass
{
  XfcePanelPluginClass parent;
};

struct _AlarmPlugin
{
  XfcePanelPlugin parent;

  GtkWidget *panel_button;
};


static void
plugin_construct(XfcePanelPlugin *panel_plugin)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
  xfce_panel_plugin_menu_show_configure (panel_plugin);
  xfce_panel_plugin_set_small (panel_plugin, TRUE);

  gtk_widget_show(plugin->panel_button);
}

static void
plugin_free_data(XfcePanelPlugin *panel_plugin)
{
  //AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);
}

static void
plugin_configure(XfcePanelPlugin *panel_plugin)
{
}


static gboolean
panel_size_changed(XfcePanelPlugin *panel_plugin, gint size)
{
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
  GtkWidget *box = gtk_bin_get_child(GTK_BIN(plugin->panel_button));

  gtk_orientable_set_orientation(GTK_ORIENTABLE(box), orientation);

  panel_size_changed(panel_plugin, xfce_panel_plugin_get_size(panel_plugin));
}

static void
panel_button_toggled(GtkWidget *panel_button, AlarmPlugin *plugin)
{
  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_button)))
    return;

  //xfce_panel_plugin_block_autohide(XFCE_PANEL_PLUGIN(plugin), TRUE);
}



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN(AlarmPlugin, alarm_plugin)

static void
alarm_plugin_class_init(AlarmPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;
  //GObjectClass *gobject_class;

  //gobject_class = G_OBJECT_CLASS (klass);
  //gobject_class->get_property = directory_menu_plugin_get_property;
  //gobject_class->set_property = directory_menu_plugin_set_property;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS(klass);
  plugin_class->construct = plugin_construct;
  plugin_class->free_data = plugin_free_data;
  plugin_class->configure_plugin = plugin_configure;
  plugin_class->size_changed = panel_size_changed;
  plugin_class->orientation_changed = panel_orientation_changed;
  //plugin_class->remote_event = plugin_remote_event;
}

static void
alarm_plugin_init(AlarmPlugin *plugin)
{
  GtkWidget *widget;

  plugin->panel_button = xfce_panel_create_toggle_button();
  xfce_panel_plugin_add_action_widget(XFCE_PANEL_PLUGIN(plugin), plugin->panel_button);
  gtk_container_add(GTK_CONTAINER(plugin), plugin->panel_button);
  gtk_widget_set_name(plugin->panel_button, "alarm-button");
  g_signal_connect(G_OBJECT(plugin->panel_button), "toggled",
                   G_CALLBACK(panel_button_toggled), plugin);

  // TODO: check if orientation-changed signal is emitted on init
  // and call orientation_changed if necessary
  widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(plugin->panel_button), widget);
  gtk_box_pack_start(GTK_BOX(widget), gtk_progress_bar_new(), TRUE, FALSE, 0);
}
