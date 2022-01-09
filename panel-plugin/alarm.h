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

#ifndef __ALARM_PLUGIN_ALARM_H__
#define __ALARM_PLUGIN_ALARM_H__

G_BEGIN_DECLS

enum AlarmId
{
  ID_UNASSIGNED = 0,
  ID_INVALID = G_MAXUINT
};

typedef enum
{
  TYPE_TIMER,
  TYPE_CLOCK,
  TYPE_COUNT
} AlarmType;

const gchar *alarm_type_icons[TYPE_COUNT];
const guint TIME_LIMITS[2*TYPE_COUNT];

enum RerunEvery
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
};

typedef enum
{
  RERUN_NDAYS = 0,
  RERUN_NWEEKS,
  RERUN_NMONTHS,
  RERUN_MODE_COUNT
} RerunMode;

typedef struct _Alarm Alarm;
struct _Alarm
{
  // Persisted settings (between plugin runtimes)
  guint id;
  AlarmType type;
  gchar *name;
  guint time;
  gchar color[8];
  gboolean autostart, autostop;
  gboolean autostart_on_resume, autostop_on_suspend;

  gint rerun_every; /* 0 (== NO_RERUN) - no rerun; >0 (> RERUN_DOW) - on days of week;
                     * <0 (< RERUN_DOW) - every N modes */
  RerunMode rerun_mode;
  Alarm *triggered_timer;

  Alert *alert; // NULL for plugin default alert
  GDateTime *timeout_at, *started_at;

  // Runtime settings
};


void alarm_free(Alarm *alarm);
GList* load_alarm_settings(AlarmPlugin *plugin);
void save_alarm_settings(AlarmPlugin *plugin, Alarm *alarm);
void save_alarm_positions(AlarmPlugin *plugin,
                          GList *alarm_iter_from, GList *alarm_iter_to);
void reset_alarm_settings(AlarmPlugin *plugin, Alarm *alarm);

G_END_DECLS

#endif /* !__ALARM_PLUGIN_ALARM_H__ */
