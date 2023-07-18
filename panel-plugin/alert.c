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

#include "alert.h"

enum AlertProperties
{
  ALERT_PROP_0,
  ALERT_PROP_NOTIFICATION,
  ALERT_PROP_SOUND,
  ALERT_PROP_SOUND_LOOPS,
  ALERT_PROP_PROGRAM,
  ALERT_PROP_PROGRAM_OPTIONS,
  ALERT_PROP_PROGRAM_RUNTIME,
  ALERT_PROP_REPEATS,
  ALERT_PROP_INTERVAL,
  ALERT_PROP_COUNT,
};

static GParamSpec *alert_class_props[ALERT_PROP_COUNT] = {NULL, };

G_DEFINE_TYPE(Alert, alert, G_TYPE_OBJECT)


// GObject definition
static void
alert_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  Alert *self = ALARM_PLUGIN_ALERT(object);

  switch (prop_id)
  {
    case ALERT_PROP_NOTIFICATION:
      g_value_set_boolean(value, self->notification);
      break;

    case ALERT_PROP_SOUND:
      g_value_set_string(value, self->sound);
      break;

    case ALERT_PROP_SOUND_LOOPS:
      g_value_set_uint(value, self->sound_loops);
      break;

    case ALERT_PROP_PROGRAM:
      g_value_set_string(value, self->program);
      break;

    case ALERT_PROP_PROGRAM_OPTIONS:
      g_value_set_string(value, self->program_options);
      break;

    case ALERT_PROP_PROGRAM_RUNTIME:
      g_value_set_uint(value, self->program_runtime);
      break;

    case ALERT_PROP_REPEATS:
      g_value_set_uint(value, self->repeats);
      break;

    case ALERT_PROP_INTERVAL:
      g_value_set_uint(value, self->interval);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}

static void
alert_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  Alert *self = ALARM_PLUGIN_ALERT(object);
  const gchar *filename;

  switch (prop_id)
  {
    case ALERT_PROP_NOTIFICATION:
      self->notification = g_value_get_boolean(value);
      break;

    case ALERT_PROP_SOUND:
      g_clear_pointer(&self->sound, g_free);

      filename = g_value_get_string(value);
      if ((filename != NULL) && g_file_test(filename, G_FILE_TEST_IS_REGULAR))
        self->sound = g_value_dup_string(value);
      break;

    case ALERT_PROP_SOUND_LOOPS:
      self->sound_loops = g_value_get_uint(value);
      break;

    case ALERT_PROP_PROGRAM:
      g_free(self->program);
      self->program = g_value_dup_string(value);
      break;

    case ALERT_PROP_PROGRAM_OPTIONS:
      g_free(self->program_options);
      self->program_options = g_value_dup_string(value);
      break;

    case ALERT_PROP_PROGRAM_RUNTIME:
      self->program_runtime = g_value_get_uint(value);
      break;

    case ALERT_PROP_REPEATS:
      self->repeats = g_value_get_uint(value);
      break;

    case ALERT_PROP_INTERVAL:
      self->interval = g_value_get_uint(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}


static void
alert_dispose(GObject *object)
{
  G_OBJECT_CLASS(alert_parent_class)->dispose(object);
}

static void
alert_finalize(GObject *object)
{
  Alert *alert = ALARM_PLUGIN_ALERT(object);

  g_free(alert->sound);
  g_free(alert->program);
  g_free(alert->program_options);

  G_OBJECT_CLASS(alert_parent_class)->finalize(object);
}


static void
alert_class_init(AlertClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  alert_class_props[ALERT_PROP_NOTIFICATION] =
    g_param_spec_boolean("notification", NULL, NULL, TRUE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[ALERT_PROP_SOUND] =
    g_param_spec_string("sound", NULL, NULL, NULL,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[ALERT_PROP_SOUND_LOOPS] =
    g_param_spec_uint("sound-loops", NULL, NULL, 0, 1000, 1,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[ALERT_PROP_PROGRAM] =
    g_param_spec_string("program", NULL, NULL, NULL,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[ALERT_PROP_PROGRAM_OPTIONS] =
    g_param_spec_string("program-options", NULL, NULL, NULL,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[ALERT_PROP_PROGRAM_RUNTIME] =
    g_param_spec_uint("program-runtime", NULL, NULL, 0, 359999, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[ALERT_PROP_REPEATS] =
    g_param_spec_uint("repeats", NULL, NULL, 0, 1000, 1,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[ALERT_PROP_INTERVAL] =
    g_param_spec_uint("interval", NULL, NULL, 0, 359999, 60,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  gobject_class->get_property = alert_get_property;
  gobject_class->set_property = alert_set_property;
  g_object_class_install_properties(gobject_class, ALERT_PROP_COUNT, alert_class_props);

  gobject_class->dispose = alert_dispose;
  gobject_class->finalize = alert_finalize;
}

static void
alert_init(Alert *alert)
{
  // Defaults set in alert_class_init param specs
}


// External interface
Alert*
alert_new(XfconfChannel *channel)
{
  Alert *alert = g_object_new(ALARM_PLUGIN_TYPE_ALERT, NULL);
  gint prop_id;
  gchar *xfconf_prop;
  GParamSpec *pspec;

  if (channel == NULL)
    return alert;

  for (prop_id = 1; prop_id < ALERT_PROP_COUNT; prop_id++)
  {
    pspec = alert_class_props[prop_id];
    xfconf_prop = g_strconcat("/", pspec->name, NULL);
    xfconf_g_property_bind(channel, xfconf_prop, pspec->value_type, alert, pspec->name);
    g_free(xfconf_prop);
  }

  return alert;
}
