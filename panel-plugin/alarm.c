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
#include <xfconf/xfconf.h>

#include "alarm.h"
#include "properties-dialog.h"

/* Plugin icon names cannot have name prefixes in common with theme icons.
 * Otherwise theme will preferably provide its own icons. */
const gchar *alarm_type_icons[TYPE_COUNT] =
{
  "xfce4-alarm-plugin-timer",
  "xfce4-alarm-plugin-clock"
};

// Utilities
void
alarm_free_func(gpointer data)
{
  Alarm *alarm = (Alarm*) data;

  g_free(alarm->uuid);
  g_free(alarm->name);
  g_date_time_unref(alarm->alert_at);
  g_slice_free(Alarm, alarm);
}

static gint
alarm_order_func(gconstpointer left, gconstpointer right)
{
  return ((Alarm*)left)->position - ((Alarm*)right)->position;
}

static GList*
load_alarm_settings(AlarmPlugin *plugin)
{
  XfcePanelPlugin *panel_plugin = XFCE_PANEL_PLUGIN(plugin);
  XfconfChannel *channel;
  const gchar *property_base;
  guint property_base_length;
  GHashTable *alarm_properties, *alarms;
  GHashTableIter alarm_property_iter;
  Alarm *alarm;
  GList *alarm_list;
  gpointer property_path, property_value;
  gchar **parts;
  guint part_count;

  g_return_val_if_fail(XFCE_IS_ALARM_PLUGIN(plugin), NULL);

  property_base = xfce_panel_plugin_get_property_base(panel_plugin);
  property_base_length = strlen(property_base);

  channel = xfce_panel_plugin_xfconf_channel_new(panel_plugin);
  alarm_properties = xfconf_channel_get_properties(channel, NULL);
  g_object_unref(channel);

  alarms = g_hash_table_new(g_str_hash, g_str_equal);

  // property_path has form: /panel/plugin-ID[[/<alarm UUID>]/<property name>]
  g_hash_table_iter_init(&alarm_property_iter, alarm_properties);
  while (g_hash_table_iter_next(&alarm_property_iter, &property_path, &property_value))
  {
    if (!g_str_has_prefix(property_path, property_base))
    {
      g_warn_if_reached();
      continue;
    }

    parts = g_strsplit((gchar*) property_path + property_base_length, "/", 3);
    part_count = g_strv_length(parts);
    if (part_count < 2 || !g_uuid_string_is_valid(parts[1]))
      goto free;

    alarm = g_hash_table_lookup(alarms, parts[1]);
    if (alarm == NULL)
    {
      alarm = g_slice_new0(Alarm);
      alarm->uuid = g_strdup(parts[1]);
      g_hash_table_insert(alarms, alarm->uuid, alarm);
    }

    if (part_count == 2)
    {
      alarm->position = g_value_get_uint(property_value);
      goto free;
    }

    if (g_strcmp0(parts[2], "type") == 0)
      alarm->type = g_value_get_uint(property_value);
    else if (g_strcmp0(parts[2], "name") == 0)
      alarm->name = g_strdup(g_value_get_string(property_value));
    else if (g_strcmp0(parts[2], "time") == 0)
      sscanf(g_value_get_string(property_value), "%02u:%02u:%02u",
             &alarm->h, &alarm->m, &alarm->s);
    else if (g_strcmp0(parts[2], "color") == 0)
      g_strlcpy(alarm->color, g_value_get_string(property_value), sizeof(alarm->color));

  free:
    g_strfreev(parts);
  }

  g_hash_table_destroy(alarm_properties);
  alarm_list = g_hash_table_get_values(alarms);
  g_hash_table_destroy(alarms);

  return g_list_sort(alarm_list, alarm_order_func);
}

void
save_alarm_settings(AlarmPlugin *plugin, Alarm *alarm)
{
  XfcePanelPlugin *panel_plugin = XFCE_PANEL_PLUGIN(plugin);
  XfconfChannel *channel;
  gchar *property_base, *value;

  g_return_if_fail(alarm != NULL);

  if (alarm->uuid == NULL)
    alarm->uuid = g_uuid_string_random();

  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));

  property_base = g_strconcat(xfce_panel_plugin_get_property_base(panel_plugin), "/",
                              alarm->uuid, NULL);
  channel = xfconf_channel_new_with_property_base(xfce_panel_get_channel_name(),
                                                  property_base);
  g_free(property_base);

  alarm->position = g_list_index(plugin->alarms, alarm);
  if (alarm->position == -1)
  {
    g_warn_if_reached();
    alarm->position = g_list_length(plugin->alarms);
  }
  g_warn_if_fail(xfconf_channel_set_uint(channel, "", alarm->position));
  g_warn_if_fail(xfconf_channel_set_uint(channel, "/type", alarm->type));
  g_warn_if_fail(xfconf_channel_set_string(channel, "/name", alarm->name));
  value = g_strdup_printf("%02u:%02u:%02u", alarm->h, alarm->m, alarm->s);
  g_warn_if_fail(xfconf_channel_set_string(channel, "/time", value));
  g_free(value);
  g_warn_if_fail(xfconf_channel_set_string(channel, "/color", alarm->color));

  g_object_unref(channel);
}

void
save_alarm_positions(AlarmPlugin *plugin, GList *alarm_iter_from, GList *alarm_iter_to)
{
  XfcePanelPlugin *panel_plugin = XFCE_PANEL_PLUGIN(plugin);
  XfconfChannel *channel;
  gchar *property_base;
  GList *alarm_iter;
  Alarm *alarm;
  gint position;

  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));
  g_return_if_fail(alarm_iter_from != NULL);
  g_return_if_fail(alarm_iter_from != alarm_iter_to);

  position = g_list_position(plugin->alarms, alarm_iter_from);
  g_return_if_fail(position != -1);

  property_base = g_strconcat(xfce_panel_plugin_get_property_base(panel_plugin), "/", NULL);
  channel = xfconf_channel_new_with_property_base(xfce_panel_get_channel_name(),
                                                  property_base);
  g_free(property_base);

  alarm_iter = alarm_iter_from;
  while (alarm_iter && (alarm_iter != alarm_iter_to))
  {
    alarm = alarm_iter->data;
    g_warn_if_fail(xfconf_channel_set_uint(channel, alarm->uuid, position));
    alarm_iter = alarm_iter->next;
    position++;
  }
  g_warn_if_fail(alarm_iter == alarm_iter_to);

  g_object_unref(channel);
}

void
reset_alarm_settings(AlarmPlugin *plugin, Alarm *alarm)
{
  XfcePanelPlugin *panel_plugin = XFCE_PANEL_PLUGIN(plugin);
  XfconfChannel *channel;
  gchar *property_base;

  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));
  g_return_if_fail(alarm != NULL);
  g_return_if_fail(alarm->uuid != NULL);

  property_base = g_strconcat(xfce_panel_plugin_get_property_base(panel_plugin), "/", NULL);
  channel = xfconf_channel_new_with_property_base(xfce_panel_get_channel_name(),
                                                  property_base);
  g_free(property_base);
  xfconf_channel_reset_property(channel, alarm->uuid, TRUE);
  g_object_unref(channel);
}

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


// Callbacks
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


// Plugin definition
XFCE_PANEL_DEFINE_PLUGIN(AlarmPlugin, alarm_plugin)

static void
plugin_construct(XfcePanelPlugin *panel_plugin)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);
  GtkWidget *box;

  xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
  xfce_panel_plugin_menu_show_configure(panel_plugin);
  xfce_panel_plugin_set_small(panel_plugin, TRUE);

  plugin->alarms = load_alarm_settings(plugin);

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
  panel_orientation_changed(panel_plugin,
                            xfce_panel_plugin_get_orientation(panel_plugin));

  gtk_widget_show_all(plugin->panel_button);
}

static void
plugin_free_data(XfcePanelPlugin *panel_plugin)
{
  AlarmPlugin *plugin = XFCE_ALARM_PLUGIN(panel_plugin);

  g_list_free_full(g_steal_pointer(&plugin->alarms), alarm_free_func);
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
  GError *error = NULL;

  if (!xfconf_init(&error))
  {
    g_critical("Failed to initialize Xfconf: %s", error->message);
    g_error_free(error);
    return;
  }
  g_object_weak_ref(G_OBJECT(plugin), (GWeakNotify) xfconf_shutdown, NULL);

  plugin->alarms = NULL;
  plugin->panel_button = NULL;
}
