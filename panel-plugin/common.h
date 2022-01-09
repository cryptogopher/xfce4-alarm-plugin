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

#ifndef __ALARM_PLUGIN_COMMON_H__
#define __ALARM_PLUGIN_COMMON_H__

#define UNICODE_INFINITY "\xe2\x88\x9e"

GtkBuilder* alarm_builder_new(XfcePanelPlugin *panel_plugin,
                              const gchar *weak_ref_id, GObject **weak_ref_obj,
                              const gchar* first_buffer, gsize first_buffer_length, ...);
void set_sensitive(GtkBuilder *builder, gboolean sensitive,
                   const gchar *first_widget_id, ...);
gint time_spin_input(GtkSpinButton *button, gdouble *new_value);
gboolean time_spin_output(GtkSpinButton *button);

void g_object_copy(GObject *src, GObject *dst);
gpointer g_object_dup(GObject *src);

#endif /* !__ALARM_PLUGIN_COMMON_H__ */
