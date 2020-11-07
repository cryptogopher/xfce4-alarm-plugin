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

void
init_alert_box(GtkBuilder *builder, XfcePanelPlugin *panel_plugin)
{
  GObject *dialog, *object, *source, *target;

  source = gtk_builder_get_object(builder, "progress");
  g_return_if_fail(GTK_IS_SWITCH(source));
  target = gtk_builder_get_object(builder, "color");
  g_return_if_fail(GTK_IS_COLOR_BUTTON(target));
  g_object_bind_property(source, "active", target, "sensitive", G_BINDING_SYNC_CREATE);
}
