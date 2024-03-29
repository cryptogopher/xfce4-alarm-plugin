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

#ifndef __ALARM_PLUGIN_ALERT_H__
#define __ALARM_PLUGIN_ALERT_H__

G_BEGIN_DECLS

enum AlertRepeats
{
  NO_ALERT_REPEAT = 0, // Alert.interval
  REPEAT_UNTIL_ACK = 0 // Alert.repeats
};

typedef struct ca_context ca_context;
struct _Alert
{
  GObject parent;

  gboolean notification;
  gchar *sound;
  guint sound_loops;
  gchar *program;
  gchar *program_options;
  guint program_runtime;
  guint repeats; // 0 (== REPEAT_UNTIL_ACK) - until acknowledged; >1 - count
  guint interval; // 0 (== NO_ALERT_REPEAT) - no repeats; >0 - every N seconds

  guint repeats_left;
};


#define ALARM_PLUGIN_TYPE_ALERT (alert_get_type())
G_DECLARE_FINAL_TYPE(Alert, alert, ALARM_PLUGIN, ALERT, GObject)

Alert* alert_new(XfconfChannel *channel);

G_END_DECLS

#endif /* !__ALARM_PLUGIN_ALERT_H__ */
