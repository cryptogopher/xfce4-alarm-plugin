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

#include <libxfce4panel/libxfce4panel.h>
#include <xfconf/xfconf.h>

#include "common.h"
#include "alert.h"
#include "alarm-plugin.h"
#include "alarm.h"

enum AlarmProperties
{
  ALARM_PROP_0,
  ALARM_PROP_TYPE,
  ALARM_PROP_NAME,
  ALARM_PROP_TIME,
  ALARM_PROP_COLOR,
  ALARM_PROP_AUTOSTART,
  ALARM_PROP_AUTOSTOP,
  ALARM_PROP_AUTOSTART_ON_RESUME,
  ALARM_PROP_AUTOSTOP_ON_SUSPEND,
  ALARM_PROP_RERUN_EVERY,
  ALARM_PROP_RERUN_MODE,
  ALARM_PROP_TRIGGERED_TIMER,
  ALARM_PROP_STARTED_AT,
  ALARM_PROP_ALERT,
  ALARM_PROP_COUNT
};

static GParamSpec *alarm_class_props[ALARM_PROP_COUNT] = {NULL, };

G_DEFINE_TYPE(Alarm, alarm, G_TYPE_OBJECT)


/* Plugin icon names cannot have name prefixes in common with theme icons.
 * Otherwise theme will preferably provide its own icons. */
const gchar *alarm_type_icons[ALARM_TYPE_COUNT] =
{
  "xfce4-alarm-plugin-timer",
  "xfce4-alarm-plugin-clock"
};

const guint TIME_LIMITS[2*ALARM_TYPE_COUNT] =
{
  1, 31622400, // Timer: 1 second to 1 year
  0, 86399 // Clock 00:00:00 to 23:59:59
};


// GObject definition
static void
alarm_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  Alarm *self = ALARM_PLUGIN_ALARM(object);

  switch (prop_id)
  {
    case ALARM_PROP_TYPE:
      g_value_set_int(value, self->type);
      break;

    case ALARM_PROP_NAME:
      g_value_set_string(value, self->name);
      break;

    case ALARM_PROP_TIME:
      g_value_set_uint(value, self->time);
      break;

    case ALARM_PROP_COLOR:
      g_value_set_boxed(value, self->color);
      break;

    case ALARM_PROP_AUTOSTART:
      g_value_set_boolean(value, self->autostart);
      break;

    case ALARM_PROP_AUTOSTOP:
      g_value_set_boolean(value, self->autostop);
      break;

    case ALARM_PROP_AUTOSTART_ON_RESUME:
      g_value_set_boolean(value, self->autostart_on_resume);
      break;

    case ALARM_PROP_AUTOSTOP_ON_SUSPEND:
      g_value_set_boolean(value, self->autostop_on_suspend);
      break;

    case ALARM_PROP_RERUN_EVERY:
      g_value_set_int(value, self->rerun_every);
      break;

    case ALARM_PROP_RERUN_MODE:
      g_value_set_int(value, self->rerun_mode);
      break;

    case ALARM_PROP_TRIGGERED_TIMER:
      g_value_set_object(value, self->triggered_timer);
      break;

    case ALARM_PROP_STARTED_AT:
      g_value_set_boxed(value, self->started_at);
      break;

    case ALARM_PROP_ALERT:
      g_value_set_object(value, self->alert);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void
alarm_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  Alarm *self = ALARM_PLUGIN_ALARM(object);
  GdkRGBA *color;

  switch (prop_id)
  {
    case ALARM_PROP_TYPE:
      self->type = CLAMP(g_value_get_int(value), 0, ALARM_TYPE_COUNT-1);
      break;

    case ALARM_PROP_NAME:
      g_free(self->name);
      self->name = g_value_dup_string(value);
      break;

    case ALARM_PROP_TIME:
      self->time = g_value_get_uint(value);
      break;

    case ALARM_PROP_COLOR:
      g_clear_pointer(&self->color, gdk_rgba_free);
      color = g_value_get_boxed(value);
      if (color)
        self->color = gdk_rgba_copy(color);
      break;

    case ALARM_PROP_AUTOSTART:
      self->autostart = g_value_get_boolean(value);
      break;

    case ALARM_PROP_AUTOSTOP:
      self->autostop = g_value_get_boolean(value);
      break;

    case ALARM_PROP_AUTOSTART_ON_RESUME:
      self->autostart_on_resume = g_value_get_boolean(value);
      break;

    case ALARM_PROP_AUTOSTOP_ON_SUSPEND:
      self->autostop_on_suspend = g_value_get_boolean(value);
      break;

    case ALARM_PROP_RERUN_EVERY:
      self->rerun_every = g_value_get_int(value);
      break;

    case ALARM_PROP_RERUN_MODE:
      self->rerun_mode = g_value_get_int(value);
      break;

    case ALARM_PROP_TRIGGERED_TIMER:
      g_object_unref(&self->triggered_timer);
      self->triggered_timer = g_value_get_object(value);
      break;

    case ALARM_PROP_STARTED_AT:
      g_date_time_unref(self->started_at);
      self->started_at = g_date_time_ref(g_value_get_boxed(value));
      break;

    case ALARM_PROP_ALERT:
      g_object_unref(&self->alert);
      self->alert = g_value_get_object(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}


static void
alarm_dispose(GObject *object)
{
  G_OBJECT_CLASS(alarm_parent_class)->dispose(object);
}

static void
alarm_finalize(GObject *object)
{
  Alarm *alarm = ALARM_PLUGIN_ALARM(object);

  g_free(alarm->name);
  gdk_rgba_free(alarm->color);
  g_object_unref(alarm->triggered_timer);
  g_date_time_unref(alarm->started_at);
  g_object_unref(alarm->alert);

  G_OBJECT_CLASS(alarm_parent_class)->finalize(object);
}


static void
alarm_class_init(AlarmClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  alarm_class_props[ALARM_PROP_TYPE] =
    g_param_spec_int("type", NULL, NULL, 0, ALARM_TYPE_COUNT-1, ALARM_TYPE_TIMER,
                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alarm_class_props[ALARM_PROP_NAME] =
    g_param_spec_string("name", NULL, NULL, NULL,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alarm_class_props[ALARM_PROP_TIME] =
    g_param_spec_uint("time", NULL, NULL, 0, 31622400, 900,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alarm_class_props[ALARM_PROP_COLOR] =
    g_param_spec_boxed("color", NULL, NULL, GDK_TYPE_RGBA,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alarm_class_props[ALARM_PROP_AUTOSTART] =
    g_param_spec_boolean("autostart", NULL, NULL, FALSE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alarm_class_props[ALARM_PROP_AUTOSTOP] =
    g_param_spec_boolean("autostop", NULL, NULL, FALSE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alarm_class_props[ALARM_PROP_AUTOSTART_ON_RESUME] =
    g_param_spec_boolean("autostart-on-resume", NULL, NULL, FALSE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alarm_class_props[ALARM_PROP_AUTOSTOP_ON_SUSPEND] =
    g_param_spec_boolean("autostop-on-suspend", NULL, NULL, FALSE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alarm_class_props[ALARM_PROP_RERUN_EVERY] =
    g_param_spec_int("rerun-every", NULL, NULL, -12000, RERUN_EVERYDAY, 0,
                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alarm_class_props[ALARM_PROP_RERUN_MODE] =
    g_param_spec_int("rerun-mode", NULL, NULL, 0, RERUN_MODE_COUNT, 0,
                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alarm_class_props[ALARM_PROP_TRIGGERED_TIMER] =
    g_param_spec_object("triggered-timer", NULL, NULL, ALARM_PLUGIN_TYPE_ALARM,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alarm_class_props[ALARM_PROP_STARTED_AT] =
    g_param_spec_boxed("started-at", NULL, NULL, G_TYPE_DATE_TIME,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alarm_class_props[ALARM_PROP_ALERT] =
    g_param_spec_object("alert", NULL, NULL, ALARM_PLUGIN_TYPE_ALERT,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  gobject_class->get_property = alarm_get_property;
  gobject_class->set_property = alarm_set_property;
  g_object_class_install_properties(gobject_class, ALARM_PROP_COUNT, alarm_class_props);

  gobject_class->dispose = alarm_dispose;
  gobject_class->finalize = alarm_finalize;
}

static void
alarm_init(Alarm *alarm)
{
  // Defaults set in alert_class_init param specs
}


// Utilities
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
  gchar *plugin_prop_base, *alarm_strid, *property_path;
  gpointer property_value;
  guint plugin_prop_base_len, alarm_id;
  gint scanned_length;
  GHashTable *alarm_properties, *alarms, *positions, *triggered_timers;
  GHashTableIter ht_iter;
  Alarm *alarm;
  GList *alarm_list;

  g_return_val_if_fail(XFCE_IS_ALARM_PLUGIN(plugin), NULL);

  plugin_prop_base = g_strconcat(xfce_panel_plugin_get_property_base(panel_plugin), "/",
                                 NULL);
  plugin_prop_base_len = strlen(plugin_prop_base);

  // alarm->id => Alarm*
  alarms = g_hash_table_new(g_int_hash, g_int_equal);
  // Alarm* => position
  positions = g_hash_table_new(NULL, NULL);
  // Alarm* => triggered_timer->id
  triggered_timers = g_hash_table_new(NULL, NULL);

  channel = xfce_panel_plugin_xfconf_channel_new(panel_plugin);
  alarm_properties = xfconf_channel_get_properties(channel, "/");
  g_object_unref(channel);

  g_hash_table_iter_init(&ht_iter, alarm_properties);
  // property_path has form: /panel/plugin-ID[[/<alarm-ID>]/<property name>]
  while (g_hash_table_iter_next(&ht_iter, (gpointer) &property_path, &property_value))
  {
    alarm_strid = property_path + plugin_prop_base_len;
    if (strstr(property_path, plugin_prop_base) != property_path ||
        strstr(property_path + plugin_prop_base_len, "/") != NULL ||
        sscanf(alarm_strid, "alarm-%u%n", &alarm_id, &scanned_length) != 1 ||
        (guint) scanned_length < strlen(alarm_strid))
      continue;

    if (g_hash_table_contains(alarms, &alarm_id))
    {
      g_warn_if_reached();
      continue;
    }

    channel = xfconf_channel_new_with_property_base(xfce_panel_get_channel_name(),
                                                    property_path);
    alarm = alarm_new(channel);
    g_object_weak_ref(G_OBJECT(alarm), (GWeakNotify) G_CALLBACK(g_object_unref), channel);

    g_hash_table_insert(alarms, &alarm->id, alarm);

    alarm->id = alarm_id;

    g_hash_table_insert(positions, alarm,
                        GUINT_TO_POINTER(g_value_get_uint(property_value)));

    alarm_id = xfconf_channel_get_uint(channel, "triggered-timer", ALARM_ID_UNASSIGNED);
    if (alarm_id != ALARM_ID_UNASSIGNED)
      g_hash_table_insert(triggered_timers, alarm, GUINT_TO_POINTER(alarm_id));

    /*
    if (!g_strcmp0(parts[2], "type"))
      alarm->type = g_value_get_uint(property_value);

    else if (!g_strcmp0(parts[2], "name"))
      alarm->name = g_strdup(g_value_get_string(property_value));

    else if (!g_strcmp0(parts[2], "time"))
      alarm->time = g_value_get_uint(property_value);

    else if (!g_strcmp0(parts[2], "color"))
    {
      if (gdk_rgba_parse(&color, g_value_get_string(property_value)))
        alarm->color = gdk_rgba_copy(&color);
    }
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
    */
  }
  g_free(plugin_prop_base);
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
  gchar *property_base, *alarm_strid;
  guint last_id;

  g_return_if_fail(alarm != NULL);

  if (alarm->id == ALARM_ID_UNASSIGNED)
  {
    last_id = ALARM_ID_UNASSIGNED;
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

  xfconf_channel_reset_property(channel, NULL, TRUE);

  position = g_list_index(plugin->alarms, alarm);
  if (position == -1)
  {
    g_warn_if_reached();
    position = g_list_length(plugin->alarms);
  }
  g_warn_if_fail(xfconf_channel_set_uint(channel, "", position));

  xfconf_channel_set_object(channel, NULL, G_OBJECT(alarm));
  if (alarm->alert)
    xfconf_channel_set_object(channel, "/alert", G_OBJECT(alarm->alert));

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

    if (alarm->id != ALARM_ID_UNASSIGNED)
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
  g_return_if_fail(alarm->id != ALARM_ID_UNASSIGNED);

  property_base = g_strconcat(xfce_panel_plugin_get_property_base(panel_plugin), "/", NULL);
  channel = xfconf_channel_new_with_property_base(xfce_panel_get_channel_name(),
                                                  property_base);
  g_free(property_base);

  alarm_strid = g_strdup_printf("alarm-%u", alarm->id);
  xfconf_channel_reset_property(channel, alarm_strid, TRUE);
  g_free(alarm_strid);

  g_object_unref(channel);
}


// External interface
Alarm*
alarm_new(XfconfChannel *channel)
{
  Alarm *alarm = g_object_new(ALARM_PLUGIN_TYPE_ALARM, NULL);
  gint prop_id;
  gchar *xfconf_prop;
  GParamSpec *pspec;

  if (channel)
    for (prop_id = 1; prop_id < ALARM_PROP_COUNT; prop_id++)
    {
      pspec = alarm_class_props[prop_id];
      xfconf_prop = g_strconcat("/", pspec->name, NULL);
      // xfconf_g_property_bind_gdkrgba
      xfconf_g_property_bind(channel, xfconf_prop, pspec->value_type, alarm, pspec->name);
      g_free(xfconf_prop);
    }

  return alarm;
}
