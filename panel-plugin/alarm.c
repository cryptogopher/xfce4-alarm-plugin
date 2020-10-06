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

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

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
panel_orientation_changed(XfcePanelPlugin *panel_plugin, GtkOrientation orientation)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);
  GtkWidget *box = gtk_bin_get_child(GTK_BIN(plugin->panel_button));

  gtk_orientable_set_orientation(GTK_ORIENTABLE(box), orientation);
}

static void
panel_button_toggled(GtkWidget *panel_button, AlarmPlugin *plugin)
{
  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_button)))
    return;

  //xfce_panel_plugin_block_autohide(XFCE_PANEL_PLUGIN(plugin), TRUE);
}


static void
alarm_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  xfce_panel_plugin_menu_show_configure (panel_plugin);

  xfce_panel_plugin_set_small (panel_plugin, TRUE);

  gtk_widget_show(plugin->panel_button);
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
  plugin_class->construct = alarm_plugin_construct;
  //plugin_class->free_data = directory_menu_plugin_free_data;
  //plugin_class->size_changed = directory_menu_plugin_size_changed;
  plugin_class->orientation_changed = panel_orientation_changed;
  //plugin_class->configure_plugin = directory_menu_plugin_configure_plugin;
  //plugin_class->remote_event = directory_menu_plugin_remote_event;
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

  // TODO: check if orientation-changed signal is emitted on init and update if not
  widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add(GTK_CONTAINER(plugin->panel_button), widget);
  gtk_box_pack_start(GTK_BOX(widget), gtk_progress_bar_new(), TRUE, FALSE, 0);
}

/*
static SamplePlugin *
sample_new (XfcePanelPlugin *plugin)
{
  SamplePlugin   *sample;
  GtkOrientation  orientation;
  GtkWidget      *label;

  // allocate memory for the plugin structure
  sample = g_slice_new0 (SamplePlugin);

  // pointer to plugin
  sample->plugin = plugin;

  // read the user settings
  sample_read (sample);

  // get the current orientation
  orientation = xfce_panel_plugin_get_orientation (plugin);

  // create some panel widgets
  sample->ebox = gtk_event_box_new ();
  gtk_widget_show (sample->ebox);

  sample->hvbox = gtk_box_new (orientation, 2);
  gtk_widget_show (sample->hvbox);
  gtk_container_add (GTK_CONTAINER (sample->ebox), sample->hvbox);

  // some sample widgets
  label = gtk_label_new (_("Sample"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (sample->hvbox), label, FALSE, FALSE, 0);

  label = gtk_label_new (_("Plugin"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (sample->hvbox), label, FALSE, FALSE, 0);

  return sample;
}



static void
sample_free (XfcePanelPlugin *plugin,
             SamplePlugin    *sample)
{
  GtkWidget *dialog;

  // check if the dialog is still open. if so, destroy it
  dialog = g_object_get_data (G_OBJECT (plugin), "dialog");
  if (G_UNLIKELY (dialog != NULL))
    gtk_widget_destroy (dialog);

  // destroy the panel widgets
  gtk_widget_destroy (sample->hvbox);

  // cleanup the settings
  if (G_LIKELY (sample->setting1 != NULL))
    g_free (sample->setting1);

  // free the plugin structure
  g_slice_free (SamplePlugin, sample);
}



static void
sample_orientation_changed (XfcePanelPlugin *plugin,
                            GtkOrientation   orientation,
                            SamplePlugin    *sample)
{
  // change the orientation of the box
  gtk_orientable_set_orientation(GTK_ORIENTABLE(sample->hvbox), orientation);
}



static gboolean
sample_size_changed (XfcePanelPlugin *plugin,
                     gint             size,
                     SamplePlugin    *sample)
{
  GtkOrientation orientation;

  // get the orientation of the plugin
  orientation = xfce_panel_plugin_get_orientation (plugin);

  // set the widget size
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);

  // we handled the orientation
  return TRUE;
}
*/
