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

#include "alert.h"
#include "alarm-plugin.h"
#include "alarm.h"

const guint TIME_LIMITS[2*TYPE_COUNT] = {
  1, 31622400, // Timer: 1 second to 1 year
  0, 86399 // Clock 00:00:00 to 23:59:59
};

/* Plugin icon names cannot have name prefixes in common with theme icons.
 * Otherwise theme will preferably provide its own icons. */
const gchar *alarm_type_icons[TYPE_COUNT] =
{
  "xfce4-alarm-plugin-timer",
  "xfce4-alarm-plugin-clock"
};


// Utilities
void
alarm_free(Alarm *alarm)
{
  g_clear_object(&alarm->alert);

  g_free(alarm->name);
  g_date_time_unref(alarm->timeout_at);
  g_slice_free(Alarm, alarm);
}

static gint
alarm_order_func(gconstpointer left, gconstpointer right, gpointer positions)
{
  return GPOINTER_TO_UINT(g_hash_table_lookup(positions, left)) -
         GPOINTER_TO_UINT(g_hash_table_lookup(positions, right));
}

GList*
load_alarm_settings(AlarmPlugin *plugin)
{
  XfcePanelPlugin *panel_plugin = XFCE_PANEL_PLUGIN(plugin);
  XfconfChannel *channel;
  const gchar *property_base;
  guint property_base_length, part_count, alarm_id;
  gint part_length;
  GHashTable *alarm_properties, *alarms, *positions, *triggered_timers;
  GHashTableIter ht_iter;
  Alarm *alarm;
  GList *alarm_list;
  gchar *property_path, **parts;
  gpointer property_value;

  g_return_val_if_fail(XFCE_IS_ALARM_PLUGIN(plugin), NULL);

  property_base = xfce_panel_plugin_get_property_base(panel_plugin);
  property_base_length = strlen(property_base);

  channel = xfce_panel_plugin_xfconf_channel_new(panel_plugin);
  alarm_properties = xfconf_channel_get_properties(channel, NULL);
  g_object_unref(channel);

  // alarm->id => Alarm*
  alarms = g_hash_table_new(g_int_hash, g_int_equal);
  // Alarm* => position
  positions = g_hash_table_new(NULL, NULL);
  // Alarm* => triggered_timer->id
  triggered_timers = g_hash_table_new(NULL, NULL);

  g_hash_table_iter_init(&ht_iter, alarm_properties);
  // property_path has form: /panel/plugin-ID[[/<alarm-ID>]/<property name>]
  while (g_hash_table_iter_next(&ht_iter, (gpointer) &property_path, &property_value))
  {
    if (!g_str_has_prefix(property_path, property_base))
    {
      g_warn_if_reached();
      continue;
    }

    parts = g_strsplit(property_path + property_base_length, "/", 3);
    part_count = g_strv_length(parts);
    if (part_count < 2 ||
        sscanf(parts[1], "alarm-%u%n", &alarm_id, &part_length) != 1 ||
        (guint)part_length < strlen(parts[1]))
      goto free;

    alarm = g_hash_table_lookup(alarms, &alarm_id);
    if (alarm == NULL)
    {
      alarm = g_slice_new0(Alarm);
      alarm->id = alarm_id;
      g_hash_table_insert(alarms, &alarm->id, alarm);
    }

    if (part_count == 2)
    {
      g_hash_table_insert(positions, alarm,
                          GUINT_TO_POINTER(g_value_get_uint(property_value)));
      goto free;
    }

    if (!g_strcmp0(parts[2], "type"))
      alarm->type = g_value_get_uint(property_value);

    else if (!g_strcmp0(parts[2], "name"))
      alarm->name = g_strdup(g_value_get_string(property_value));

    else if (!g_strcmp0(parts[2], "time"))
      alarm->time = g_value_get_uint(property_value);

    else if (!g_strcmp0(parts[2], "color"))
      g_strlcpy(alarm->color, g_value_get_string(property_value), sizeof(alarm->color));

    else if (!g_strcmp0(parts[2], "autostart"))
      alarm->autostart = g_value_get_boolean(property_value);

    else if (!g_strcmp0(parts[2], "autostop"))
      alarm->autostop = g_value_get_boolean(property_value);

    else if (!g_strcmp0(parts[2], "autostart-on-resume"))
      alarm->autostart_on_resume = g_value_get_boolean(property_value);

    else if (!g_strcmp0(parts[2], "autostop-on-suspend"))
      alarm->autostop_on_suspend = g_value_get_boolean(property_value);

    else if (!g_strcmp0(parts[2], "triggered-timer"))
      g_hash_table_insert(triggered_timers, alarm,
                          GUINT_TO_POINTER(g_value_get_uint(property_value)));

    else if (!g_strcmp0(parts[2], "rerun-every"))
      alarm->rerun_every = g_value_get_int(property_value);

    else if (!g_strcmp0(parts[2], "rerun-mode"))
      alarm->rerun_mode = g_value_get_uint(property_value);

  free:
    g_strfreev(parts);
  }
  g_hash_table_destroy(alarm_properties);

  /* TODO: consistency checks and coercion/alarm removal?
   * e.g. type/triggered-timer, type/rerun-every, rerun-every/rerun-mode */

  g_hash_table_iter_init(&ht_iter, triggered_timers);
  while (g_hash_table_iter_next(&ht_iter, (gpointer) &alarm, (gpointer) &alarm_id))
  {
    alarm->triggered_timer = g_hash_table_lookup(alarms, &alarm_id);
    g_warn_if_fail(alarm->triggered_timer != NULL);
  }
  g_hash_table_destroy(triggered_timers);

  alarm_list = g_hash_table_get_values(alarms);
  g_hash_table_destroy(alarms);

  alarm_list = g_list_sort_with_data(alarm_list, alarm_order_func, positions);
  g_hash_table_destroy(positions);
  return alarm_list;
}

void
save_alarm_settings(AlarmPlugin *plugin, Alarm *alarm)
{
  XfcePanelPlugin *panel_plugin = XFCE_PANEL_PLUGIN(plugin);
  XfconfChannel *channel;
  GList *alarm_iter;
  gint position;
  gchar *property_base;
  GParamSpec **specs;
  guint spec_count, i, last_id;
  gchar *property_name, *alarm_strid;
  GValue property_value = G_VALUE_INIT;

  g_return_if_fail(alarm != NULL);

  if (alarm->id == ID_UNASSIGNED)
  {
    last_id = ID_UNASSIGNED;
    alarm_iter = plugin->alarms;
    while (alarm_iter)
    {
      last_id = MAX(((Alarm*) alarm_iter->data)->id, last_id);
      alarm_iter = alarm_iter->next;
    }
    alarm->id = last_id + 1;
  }

  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));

  alarm_strid = g_strdup_printf("alarm-%u", alarm->id);
  property_base = g_strconcat(xfce_panel_plugin_get_property_base(panel_plugin), "/",
                              alarm_strid, NULL);
  g_free(alarm_strid);
  channel = xfconf_channel_new_with_property_base(xfce_panel_get_channel_name(),
                                                  property_base);
  g_free(property_base);

  position = g_list_index(plugin->alarms, alarm);
  if (position == -1)
  {
    g_warn_if_reached();
    position = g_list_length(plugin->alarms);
  }
  g_warn_if_fail(xfconf_channel_set_uint(channel, "", position));
  g_warn_if_fail(xfconf_channel_set_uint(channel, "/type", alarm->type));
  g_warn_if_fail(xfconf_channel_set_string(channel, "/name", alarm->name));
  g_warn_if_fail(xfconf_channel_set_uint(channel, "/time", alarm->time));
  g_warn_if_fail(xfconf_channel_set_string(channel, "/color", alarm->color));
  g_warn_if_fail(xfconf_channel_set_bool(channel, "/autostart", alarm->autostart));
  g_warn_if_fail(xfconf_channel_set_bool(channel, "/autostop", alarm->autostop));
  g_warn_if_fail(xfconf_channel_set_bool(channel, "/autostart-on-resume",
                                         alarm->autostart_on_resume));
  g_warn_if_fail(xfconf_channel_set_bool(channel, "/autostop-on-suspend",
                                         alarm->autostop_on_suspend));

  xfconf_channel_reset_property(channel, "/triggered-timer", FALSE);
  xfconf_channel_reset_property(channel, "/rerun-every", FALSE);
  xfconf_channel_reset_property(channel, "/rerun-mode", FALSE);
  if (alarm->type == TYPE_TIMER)
  {
    if (alarm->triggered_timer != NULL)
      g_warn_if_fail(xfconf_channel_set_uint(channel, "/triggered-timer",
                                             alarm->triggered_timer->id));
  }
  else 
  {
    if (alarm->rerun_every != NO_RERUN)
    {
      g_warn_if_fail(xfconf_channel_set_int(channel, "/rerun-every", alarm->rerun_every));
      if (alarm->rerun_every < RERUN_DOW)
        g_warn_if_fail(xfconf_channel_set_uint(channel, "/rerun-mode", alarm->rerun_mode));
    }
  }

  xfconf_channel_reset_property(channel, "/alert", TRUE);
  if (alarm->alert != NULL)
  {
    specs = g_object_class_list_properties(G_OBJECT_GET_CLASS(alarm->alert), &spec_count);

    for (i = 0; i < spec_count; i++)
    {
      if ((specs[i]->flags & G_PARAM_READWRITE) == 0)
        continue;

      property_name = g_strconcat("/alert/", g_param_spec_get_name(specs[i]), NULL);
      g_object_get_property(G_OBJECT(alarm->alert), g_param_spec_get_name(specs[i]),
                            &property_value);
      xfconf_channel_set_property(channel, property_name, &property_value);
      g_free(property_name);
      g_value_unset(&property_value);
    }
  }

  g_object_unref(channel);
}

void
save_alarm_positions(AlarmPlugin *plugin, GList *alarm_iter_from, GList *alarm_iter_to)
{
  XfcePanelPlugin *panel_plugin = XFCE_PANEL_PLUGIN(plugin);
  XfconfChannel *channel;
  gchar *property_base, *alarm_strid;
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

    if (alarm->id != ID_UNASSIGNED)
    {
      alarm_strid = g_strdup_printf("alarm-%u", alarm->id);
      g_warn_if_fail(xfconf_channel_set_uint(channel, alarm_strid, position));
      g_free(alarm_strid);
    }
    else
      g_warn_if_reached();

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
  gchar *property_base, *alarm_strid;

  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));
  g_return_if_fail(alarm != NULL);
  g_return_if_fail(alarm->id != ID_UNASSIGNED);

  property_base = g_strconcat(xfce_panel_plugin_get_property_base(panel_plugin), "/", NULL);
  channel = xfconf_channel_new_with_property_base(xfce_panel_get_channel_name(),
                                                  property_base);
  g_free(property_base);

  alarm_strid = g_strdup_printf("alarm-%u", alarm->id);
  xfconf_channel_reset_property(channel, alarm_strid, TRUE);
  g_free(alarm_strid);

  g_object_unref(channel);
}
