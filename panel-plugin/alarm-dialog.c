/*
 *  Copyright (C) 2019 John Doo <john@foo.org>
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

#include <libxfce4panel/xfce-panel-plugin.h>

#include "alarm-dialog.h"
#include "alarm-dialog_ui.h"

static void
alarm_to_dialog(Alarm *alarm, GtkBuilder *builder)
{
}

static void
alarm_from_dialog(Alarm *alarm, GtkBuilder *builder)
{
}

void
show_alarm_dialog(GtkWidget *parent, AlarmPlugin *plugin, Alarm **alarm)
{
  GtkBuilder *builder;
  GObject *dialog;

  g_return_if_fail(GTK_IS_WINDOW(parent));
  g_return_if_fail(XFCE_IS_ALARM_PLUGIN(plugin));
  g_return_if_fail(alarm != NULL);

  builder = alarm_builder_new(XFCE_PANEL_PLUGIN(plugin),
                              alarm_dialog_ui, alarm_dialog_ui_length);
  g_return_if_fail(GTK_IS_BUILDER(builder));

  dialog = gtk_builder_get_object(builder, "alarm-dialog");
  g_return_if_fail(GTK_IS_DIALOG(dialog));
  xfce_panel_plugin_take_window(XFCE_PANEL_PLUGIN(plugin), GTK_WINDOW(dialog));
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));

  //gtk_builder_connect_signals(builder, NULL);

  if (*alarm)
    alarm_to_dialog(*alarm, builder);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_APPLY)
  {
    if (*alarm == NULL)
      *alarm = g_slice_new0(Alarm);

    alarm_from_dialog(*alarm, builder);
  }

  g_object_unref(builder);
}
