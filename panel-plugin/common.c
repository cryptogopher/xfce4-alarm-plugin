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
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>

#include "common.h"
#include "alert.h"
#include "alarm-plugin.h"
#include "alarm.h"

// GTK
GtkBuilder*
alarm_builder_new(XfcePanelPlugin *panel_plugin,
                  const gchar *weak_ref_id, GObject **weak_ref_obj,
                  const gchar* first_buffer, gsize first_buffer_length, ...)
{
  GtkBuilder *builder;
  GError *error = NULL;
  va_list var_args;
  const gchar *buffer = first_buffer;
  gsize buffer_length = first_buffer_length;

  g_return_val_if_fail(XFCE_IS_PANEL_PLUGIN(panel_plugin), NULL);
  g_return_val_if_fail(first_buffer != NULL, NULL);
  g_return_val_if_fail(first_buffer_length > 0, NULL);

  /* Hack to make sure GtkBuilder knows about the XfceTitledDialog object
   * https://wiki.xfce.org/releng/4.8/roadmap/libxfce4ui
   * http://bugzilla.gnome.org/show_bug.cgi?id=588238 */
  if (xfce_titled_dialog_get_type() == 0) return NULL;

  builder = gtk_builder_new();

  va_start(var_args, first_buffer_length);
  while (buffer != NULL)
  {
    if (!gtk_builder_add_from_string(builder, buffer, buffer_length, &error))
    {
      g_critical("Failed to construct the builder for plugin %s-%d: %s.",
                 xfce_panel_plugin_get_name (panel_plugin),
                 xfce_panel_plugin_get_unique_id (panel_plugin),
                 error->message);
      g_error_free(error);
      g_object_unref(builder);
      return NULL;
    }

    buffer = va_arg(var_args, gchar*);
    if (buffer != NULL)
      buffer_length = va_arg(var_args, gsize);
  }
  va_end(var_args);

  *weak_ref_obj = gtk_builder_get_object(builder, weak_ref_id);
  if (GTK_IS_WIDGET(*weak_ref_obj))
    g_object_weak_ref(*weak_ref_obj, (GWeakNotify) G_CALLBACK(g_object_unref), builder);
  else
    g_clear_object(&builder);

  return builder;
}

void
set_sensitive(GtkBuilder *builder, gboolean sensitive, const gchar *first_widget_id, ...)
{
  va_list var_args;
  const gchar *widget_id = first_widget_id;
  GObject *object;

  g_return_if_fail(GTK_IS_BUILDER(builder));
  g_return_if_fail(first_widget_id != NULL);

  va_start(var_args, first_widget_id);
  while (widget_id != NULL)
  {
    object = gtk_builder_get_object(builder, widget_id);
    g_return_if_fail(GTK_IS_WIDGET(object));
    gtk_widget_set_sensitive(GTK_WIDGET(object), sensitive);

    widget_id = va_arg(var_args, gchar*);
  }
  va_end(var_args);
}

gint
time_spin_input(GtkSpinButton *button, gdouble *new_value)
{
  const gchar *time;
  gchar *error = NULL, **parts;
  gint pos = 0;
  gboolean zero_inf;

  g_return_val_if_fail(GTK_IS_SPIN_BUTTON(button), FALSE);

  time = gtk_entry_get_text(GTK_ENTRY(button));
  if (time == NULL)
    return GTK_INPUT_ERROR;

  zero_inf = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "zero-is-infinity"));
  if (zero_inf && !g_strcmp0(time, UNICODE_INFINITY))
  {
    *new_value = 0;
    return TRUE;
  }

  parts = g_strsplit_set(time, ":,.;- ", 3);
  do
  {
    *new_value = 60*(*new_value) + g_strtod(parts[pos], &error);
    pos++;
  }
  while ((parts[pos] != NULL) && (*error == '\0'));
  g_strfreev(parts);

  *new_value *= pow(60, 3 - pos);

  if (error != NULL)
    return GTK_INPUT_ERROR;
  else
    return TRUE;
}

gboolean
time_spin_output(GtkSpinButton *button)
{
  gint value;
  gchar *time;
  gboolean zero_inf;

  g_return_val_if_fail(GTK_IS_SPIN_BUTTON(button), FALSE);

  value = gtk_spin_button_get_value(button);
  zero_inf = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "zero-is-infinity"));
  if (zero_inf && value == 0)
    time = g_strdup(UNICODE_INFINITY);
  else
    time = g_strdup_printf("%02u:%02u:%02u", value/3600, value%3600/60, value%60);

  if (g_strcmp0(time, gtk_entry_get_text(GTK_ENTRY(button))))
    gtk_entry_set_text(GTK_ENTRY(button), time);
  g_free(time);

  return TRUE;
}


// GObject
void
g_object_copy(GObject *src, GObject *dst)
{
  GParamSpec **specs;
  guint spec_count, i, prop_count = 0;
  const gchar **names;
  GValue *values;

  if (!src) return;

  g_return_if_fail(G_IS_OBJECT(src));
  g_return_if_fail(G_IS_OBJECT(dst));
  g_return_if_fail(G_TYPE_FROM_INSTANCE(src) == G_TYPE_FROM_INSTANCE(dst));

  specs = g_object_class_list_properties(G_OBJECT_GET_CLASS(src), &spec_count);

  names = g_new0(const gchar*, spec_count);
  values = g_new0(GValue, spec_count);
  for (i = 0; i < spec_count; i++)
  {
    if ((specs[i]->flags & G_PARAM_READWRITE) == 0)
      continue;

    names[prop_count] = g_param_spec_get_name(specs[i]);
    g_object_get_property(src, names[prop_count], &values[prop_count]);

    prop_count++;
  }

  g_object_setv(dst, prop_count, names, values);

  g_free(specs);
  g_free(names);
  for (i = 0; i < prop_count; i++)
    g_value_unset(&values[i]);
  g_free(values);

  return;
}

gpointer
g_object_dup(GObject *src)
{
  GObject *dst;

  if (!src) return NULL;

  g_return_val_if_fail(G_IS_OBJECT(src), NULL);

  dst = g_object_new(G_TYPE_FROM_INSTANCE(src), NULL);
  g_object_copy(src, dst);
  return (gpointer) dst;
}


// Xfconf
void
xfconf_channel_set_object(XfconfChannel *channel, const gchar *prefix, GObject *object)
{
  GParamSpec **specs;
  guint spec_count, i;
  gchar *prop_name;
  GValue prop_value = G_VALUE_INIT;

  g_return_if_fail(object != NULL);

  specs = g_object_class_list_properties(G_OBJECT_GET_CLASS(object), &spec_count);

  for (i = 0; i < spec_count; i++)
  {
    if ((specs[i]->flags & G_PARAM_READWRITE) == 0)
      continue;

    prop_name = g_strdup_printf("%s/%s", prefix, g_param_spec_get_name(specs[i]));
    g_object_get_property(object, g_param_spec_get_name(specs[i]), &prop_value);

    if (specs[i]->value_type == ALARM_PLUGIN_TYPE_ALARM)
      ;
    else if (specs[i]->value_type == ALARM_PLUGIN_TYPE_ALERT)
      xfconf_channel_set_object(channel, "/alert", g_value_get_object(&prop_value));
    else
      g_warn_if_fail(xfconf_channel_set_property(channel, prop_name, &prop_value));

    g_free(prop_name);
    g_value_unset(&prop_value);
  }

  g_free(specs);
}
