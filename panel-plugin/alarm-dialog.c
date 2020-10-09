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

void show_alarm_dialog(AlarmPlugin *plugin)
{
  //GtkWidget *dialog;

  /* block the plugin menu */
  //xfce_panel_plugin_block_menu (plugin);

  /* create the dialog */
  //dialog = xfce_titled_dialog_new_with_buttons (_("Sample Plugin"),
    //                                            GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
      //                                          GTK_DIALOG_DESTROY_WITH_PARENT,
        //                                        "gtk-help", GTK_RESPONSE_HELP,
          //                                      "gtk-close", GTK_RESPONSE_OK,
            //                                    NULL);

  /* center dialog on the screen */
  //gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  /* set dialog icon */
  //gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");

  /* link the dialog to the plugin, so we can destroy it when the plugin
   * is closed, but the dialog is still open */
  //g_object_set_data (G_OBJECT (plugin), "dialog", dialog);

  /* connect the response signal to the dialog */
  //g_signal_connect (G_OBJECT (dialog), "response",
  //                  G_CALLBACK(sample_configure_response), sample);

  /* show the entire dialog */
  //gtk_widget_show (dialog);
}
