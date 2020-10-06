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

#ifndef __XFCE_ALARM_PLUGIN_H__
#define __XFCE_ALARM_PLUGIN_H__

G_BEGIN_DECLS

typedef struct _AlarmPluginClass AlarmPluginClass;
typedef struct _AlarmPlugin      AlarmPlugin;

#define XFCE_TYPE_ALARM_PLUGIN (alarm_plugin_get_type ())
#define XFCE_ALARM_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_ALARM_PLUGIN, AlarmPlugin))
#define XFCE_ALARM_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_ALARM_PLUGIN, AlarmPluginClass))
#define XFCE_IS_ALARM_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_ALARM_PLUGIN))
#define XFCE_IS_ALARM_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_ALARM_PLUGIN))
#define XFCE_ALARM_PLUGIN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_ALARM_PLUGIN, AlarmPluginClass))

GType alarm_plugin_get_type(void) G_GNUC_CONST;

void alarm_plugin_register_type(XfcePanelTypeModule *type_module);

/* plugin structure */
typedef struct
{
    XfcePanelPlugin *plugin;

    /* panel widgets */
    GtkWidget       *ebox;
    GtkWidget       *hvbox;
    GtkWidget       *label;

    /* sample settings */
    gchar           *setting1;
    gint             setting2;
    gboolean         setting3;
}
SamplePlugin;

G_END_DECLS

#endif /* !__XFCE_ALARM_PLUGIN_H__ */
