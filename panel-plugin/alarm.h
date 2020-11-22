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

#ifndef __ALARM_PLUGIN_H__
#define __ALARM_PLUGIN_H__

G_BEGIN_DECLS

typedef struct _AlarmPluginClass AlarmPluginClass;
typedef struct _AlarmPlugin AlarmPlugin;
typedef struct _Alert Alert;
typedef struct _Alarm Alarm;

struct _AlarmPluginClass
{
  XfcePanelPluginClass parent;
};

// Only store things that have lifetime of the plugin here
struct _AlarmPlugin
{
  XfcePanelPlugin parent;

  GList *alarms;
  Alert *alert;
  GtkWidget *panel_button;
};

typedef enum
{
  TYPE_TIMER,
  TYPE_CLOCK,
  TYPE_COUNT
} AlarmType;

enum
{
  NO_RERUN        = 0,
  RERUN_DOW       = 0,
  RERUN_MONDAY    = 1,
  RERUN_TUEASDAY  = 1 << 1,
  RERUN_WEDNESDAY = 1 << 2,
  RERUN_THURSDAY  = 1 << 3,
  RERUN_FRIDAY    = 1 << 4,
  RERUN_SATURDAY  = 1 << 5,
  RERUN_SUNDAY    = 1 << 6,
  RERUN_WEEKDAY   = 31,
  RERUN_WEEKEND   = 96,
  RERUN_EVERYDAY  = 127
}; // RerunEvery

typedef enum
{
  RERUN_NDAYS = 0,
  RERUN_NWEEKS,
  RERUN_NMONTHS,
  RERUN_MODE_COUNT
} RerunMode;

enum
{
  NO_ALERT_REPEAT = 0, // Alert.interval
  REPEAT_UNTIL_ACK = 0 // Alert.repeats
};

struct _Alert
{
  gboolean notification;
  gchar *sound;
  guint sound_loops;
  gchar *program;
  gchar *program_options;
  guint program_runtime;
  guint interval; // 0 (== NO_ALERT_REPEAT) - no repeats; >0 - every N seconds
  guint repeats; // 0 (== REPEAT_UNTIL_ACK) - until acknowledged; >1 - count

  guint repeats_left;
};

struct _Alarm
{
  // Persisted settings (between plugin runtimes)
  gchar *uuid;
  AlarmType type;
  gchar *name;
  guint h, m, s;
  gchar color[8];
  gboolean autostart, autostop;
  gboolean autostart_on_resume, autostop_on_suspend;

  gint rerun_every; /* 0 (== NO_RERUN) - no rerun; >0 (> RERUN_DOW) - on days of week;
                     * <0 (< RERUN_DOW) - every N modes */
  RerunMode rerun_mode;
  Alarm *triggered_timer;

  Alert *alert;
  GDateTime *timeout_at;
  GTimer *alert_timer;

  // Runtime settings
};

// Column numbers are used in .glade - update if changed
enum AlarmColumns
{
  COL_DATA,
  COL_ICON_NAME,
  COL_TIME,
  COL_COLOR,
  COL_NAME,
  COL_COUNT
};

const gchar *alarm_type_icons[TYPE_COUNT];


#define XFCE_TYPE_ALARM_PLUGIN (alarm_plugin_get_type ())
#define XFCE_ALARM_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_ALARM_PLUGIN, AlarmPlugin))
#define XFCE_ALARM_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_ALARM_PLUGIN, AlarmPluginClass))
#define XFCE_IS_ALARM_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_ALARM_PLUGIN))
#define XFCE_IS_ALARM_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_ALARM_PLUGIN))
#define XFCE_ALARM_PLUGIN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_ALARM_PLUGIN, AlarmPluginClass))

GType alarm_plugin_get_type(void) G_GNUC_CONST;
void alarm_plugin_register_type(XfcePanelTypeModule *type_module);

void alarm_free_func(gpointer data);
void save_alarm_settings(AlarmPlugin *plugin, Alarm *alarm);
void save_alarm_positions(AlarmPlugin *plugin,
                          GList *alarm_iter_from, GList *alarm_iter_to);
void reset_alarm_settings(AlarmPlugin *plugin, Alarm *alarm);
GtkBuilder* alarm_builder_new(XfcePanelPlugin *panel_plugin, const gchar *weak_ref_id,
                              const gchar* first_buffer, gsize first_buffer_length, ...);
void set_sensitive(GtkBuilder *builder, gboolean sensitive,
                   const gchar *first_widget_id, ...);

G_END_DECLS

#endif /* !__ALARM_PLUGIN_H__ */
