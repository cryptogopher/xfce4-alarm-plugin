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

#include <math.h>

#include <canberra.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <xfconf/xfconf.h>
#include <exo/exo.h>

#include "alarm.h"
#include "alert-box_ui.h"
#include "alert.h"

#define UNICODE_INFINITY "\xe2\x88\x9e"

struct _Alert
{
  GObject parent;
  GtkBuilder *builder;
  ca_context *context;

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

enum AlertRepeats
{
  NO_ALERT_REPEAT = 0, // Alert.interval
  REPEAT_UNTIL_ACK = 0 // Alert.repeats
};

enum AlertProperties
{
  PROP_0,
  PROP_ALERT_NOTIFICATION,
  PROP_ALERT_SOUND,
  PROP_ALERT_SOUND_LOOPS,
  PROP_ALERT_PROGRAM,
  PROP_ALERT_PROGRAM_OPTIONS,
  PROP_ALERT_PROGRAM_RUNTIME,
  PROP_ALERT_REPEATS,
  PROP_ALERT_INTERVAL,
  PROP_COUNT,
};

static GParamSpec *alert_class_props[PROP_COUNT] = {NULL, };

enum ProgramColumns
{
  PROGRAM_DATA,
  PROGRAM_ICON,
  PROGRAM_NAME,
  PROGRAM_SEPARATOR,
  PROGRAM_HAS_ICON
};

enum ProgramEntries
{
  PROGRAM_NONE = 0,
  PROGRAM_CHOOSE_FILE = 2
};


G_DEFINE_TYPE(Alert, alert, G_TYPE_OBJECT)


// GObject definition
static void
alert_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  Alert *self = ALARM_PLUGIN_ALERT(object);

  switch (prop_id)
  {
    case PROP_ALERT_NOTIFICATION:
      g_value_set_boolean(value, self->notification);
      break;

    case PROP_ALERT_SOUND:
      g_value_set_string(value, self->sound);
      break;

    case PROP_ALERT_SOUND_LOOPS:
      g_value_set_uint(value, self->sound_loops);
      break;

    case PROP_ALERT_PROGRAM:
      g_value_set_string(value, self->program);
      break;

    case PROP_ALERT_PROGRAM_OPTIONS:
      g_value_set_string(value, self->program_options);
      break;

    case PROP_ALERT_PROGRAM_RUNTIME:
      g_value_set_uint(value, self->program_runtime);
      break;

    case PROP_ALERT_REPEATS:
      g_value_set_uint(value, self->repeats);
      break;

    case PROP_ALERT_INTERVAL:
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
    case PROP_ALERT_NOTIFICATION:
      self->notification = g_value_get_boolean(value);
      break;

    case PROP_ALERT_SOUND:
      g_clear_pointer(&self->sound, g_free);

      filename = g_value_get_string(value);
      if ((filename != NULL) && g_file_test(filename, G_FILE_TEST_IS_REGULAR))
        self->sound = g_value_dup_string(value);
      break;

    case PROP_ALERT_SOUND_LOOPS:
      self->sound_loops = g_value_get_uint(value);
      break;

    case PROP_ALERT_PROGRAM:
      g_free(self->program);
      self->program = g_value_dup_string(value);
      break;

    case PROP_ALERT_PROGRAM_OPTIONS:
      g_free(self->program_options);
      self->program_options = g_value_dup_string(value);
      break;

    case PROP_ALERT_PROGRAM_RUNTIME:
      self->program_runtime = g_value_get_uint(value);
      break;

    case PROP_ALERT_REPEATS:
      self->repeats = g_value_get_uint(value);
      break;

    case PROP_ALERT_INTERVAL:
      self->interval = g_value_get_uint(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
  }
}


static void
alert_dispose(GObject *object)
{
  // unref
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

  alert_class_props[PROP_ALERT_NOTIFICATION] =
    g_param_spec_boolean("notification", NULL, NULL, TRUE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[PROP_ALERT_SOUND] =
    g_param_spec_string("sound", NULL, NULL, "",
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[PROP_ALERT_SOUND_LOOPS] =
    g_param_spec_uint("sound-loops", NULL, NULL, 0, 1000, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[PROP_ALERT_PROGRAM] =
    g_param_spec_string("program", NULL, NULL, "",
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[PROP_ALERT_PROGRAM_OPTIONS] =
    g_param_spec_string("program-options", NULL, NULL, "",
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[PROP_ALERT_PROGRAM_RUNTIME] =
    g_param_spec_uint("program-runtime", NULL, NULL, 0, 359999, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[PROP_ALERT_REPEATS] =
    g_param_spec_uint("repeats", NULL, NULL, 0, 1000, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  alert_class_props[PROP_ALERT_INTERVAL] =
    g_param_spec_uint("interval", NULL, NULL, 0, 359999, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  gobject_class->get_property = alert_get_property;
  gobject_class->set_property = alert_set_property;
  g_object_class_install_properties(gobject_class, PROP_COUNT, alert_class_props);

  gobject_class->dispose = alert_dispose;
  gobject_class->finalize = alert_finalize;
}

static void
alert_init(Alert *alert)
{
  alert->notification = TRUE;
  alert->sound = NULL;
  alert->sound_loops = 1;
  alert->program = NULL;
  alert->program_options = NULL;
  alert->program_runtime = 0;
  alert->repeats = 1;
  alert->interval = 60;
}


// Utilities
static gboolean playback_finished(gpointer button);
static void ca_playback_finished(ca_context *c, uint32_t id, int error_code, void *button);

static void
clear_context(Alert *alert)
{
  g_return_if_fail(alert->context != NULL);

  ca_context_destroy(alert->context);
  alert->context = NULL;
}

static void
ca_playback_finished(ca_context *c, uint32_t id, int error_code, void *button)
{
  if (error_code != CA_ERROR_CANCELED)
    g_idle_add(playback_finished, button);
}

static gint
app_order_func(gconstpointer left, gconstpointer right)
{
  // TODO: make sorting case insensitive
  return g_utf8_collate(g_app_info_get_display_name(G_APP_INFO(left)),
                        g_app_info_get_display_name(G_APP_INFO(right)));
}

static gchar*
show_program_dialog(GtkBuilder *builder, GtkWidget *parent)
{
  GObject *dialog;
  gchar *filename = NULL;

  dialog = gtk_builder_get_object(builder, "program-dialog");
  g_return_val_if_fail(GTK_IS_FILE_CHOOSER_DIALOG(dialog), PROGRAM_NONE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

  gtk_widget_hide(GTK_WIDGET(dialog));

  return filename;
}

static int
select_program_by_cmdline(GtkTreeModel *model, const gchar *filename)
{
  gint active = PROGRAM_NONE;
  GAppInfo *appinfo, *tree_appinfo;
  GError *error = NULL;
  GtkTreeIter iter;
  GtkTreePath *path;
  GFile *file;
  GFileInfo *fileinfo;
  GIcon *fileicon = NULL;

  if (exo_str_is_empty(filename))
    return PROGRAM_NONE;

  appinfo = g_app_info_create_from_commandline(filename, NULL, G_APP_INFO_CREATE_NONE,
                                               &error);
  if (error)
  {
    g_warning("Failed to get app info: %s.", error->message);
    g_error_free(error);
    return PROGRAM_NONE;
  }

  // Look for existing entry with given commandline and select if already added
  if (gtk_tree_model_get_iter_first(model, &iter))
    do
    {
      gtk_tree_model_get(model, &iter, PROGRAM_DATA, &tree_appinfo, -1);
      if (tree_appinfo != NULL &&
          !g_strcmp0(g_app_info_get_commandline(appinfo),
                     g_app_info_get_commandline(tree_appinfo)))
      {
        path = gtk_tree_model_get_path(model, &iter);
        active = gtk_tree_path_get_indices(path)[0];
        gtk_tree_path_free(path);

        g_clear_object(&appinfo);
        break;
      }
    }
    while (gtk_tree_model_iter_next(model, &iter));

  // Otherwise insert new entry right below 'Choose program file'
  if (active == PROGRAM_NONE)
  {
    active = PROGRAM_CHOOSE_FILE + 1;

    file = g_file_new_for_path(filename);
    fileinfo = g_file_query_info(file, G_FILE_ATTRIBUTE_STANDARD_ICON,
        G_FILE_QUERY_INFO_NONE, NULL, NULL);
    if (fileinfo != NULL)
      fileicon = g_file_info_get_icon(fileinfo);

    gtk_list_store_insert_with_values(GTK_LIST_STORE(model), NULL, active,
                                    PROGRAM_DATA, appinfo,
                                    PROGRAM_ICON, fileicon,
                                    PROGRAM_NAME, g_app_info_get_display_name(appinfo),
                                    PROGRAM_SEPARATOR, FALSE,
                                    PROGRAM_HAS_ICON, TRUE, -1);

    g_object_unref(fileinfo);
    g_object_unref(file);
  }

  return active;
}


// Callbacks
static void
sound_chooser_selection_changed(GtkFileChooserButton *button, Alert *alert)
{
  gchar *filename;

  g_return_if_fail(GTK_IS_FILE_CHOOSER_BUTTON(button));
  g_return_if_fail(ALARM_PLUGIN_IS_ALERT(alert));

  filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(button));
  g_object_set(G_OBJECT(alert), "sound", filename, NULL);
  if ((filename != NULL) && (alert->sound == NULL))
    gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(button));
  g_free(filename);

  set_sensitive(alert->builder, alert->sound != NULL,
                "sound-play-box", "sound-loop-box", NULL);
}

static void
clear_sound_clicked(GtkButton *button, Alert *alert)
{
  GObject *object;

  g_return_if_fail(GTK_IS_BUTTON(button));
  g_return_if_fail(ALARM_PLUGIN_IS_ALERT(alert));

  object = gtk_builder_get_object(alert->builder, "sound-chooser");
  g_return_if_fail(GTK_IS_FILE_CHOOSER(object));
  gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(object));
}

static void
play_sound_toggled(GtkToggleButton *button, Alert *alert)
{
  gboolean play;
  GObject *image;
  ca_proplist *proplist;
  int is_playing;

  g_return_if_fail(GTK_IS_TOGGLE_BUTTON(button));
  g_return_if_fail(ALARM_PLUGIN_IS_ALERT(alert));

  play = gtk_toggle_button_get_active(button);

  set_sensitive(alert->builder, !play,
                "sound-chooser", "clear-sound", "sound-loop-box", NULL);

  image = gtk_builder_get_object(alert->builder, play ? "image-stop" : "image-play");
  g_return_if_fail(GTK_IS_IMAGE(image));
  gtk_button_set_image(GTK_BUTTON(button), GTK_WIDGET(image));
  gtk_toggle_button_set_active(button, play);

  if (play)
  {
    g_warn_if_fail(!ca_proplist_create(&proplist));
    g_warn_if_fail(!ca_proplist_sets(proplist, CA_PROP_MEDIA_FILENAME, alert->sound));
    g_warn_if_fail(!ca_context_play_full(alert->context, 1, proplist, ca_playback_finished,
                                         button));
    g_warn_if_fail(!ca_proplist_destroy(proplist));
  }
  else
  {
    g_warn_if_fail(!ca_context_playing(alert->context, 1, &is_playing));
    if (is_playing)
      g_warn_if_fail(!ca_context_cancel(alert->context, 1));
  }
}

static gboolean
playback_finished(gpointer button)
{
  g_return_val_if_fail(GTK_IS_TOGGLE_BUTTON(button), G_SOURCE_REMOVE);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);

  return G_SOURCE_REMOVE;
}

static gboolean
count_spin_output(GtkSpinButton *button, gpointer user_data)
{
  GtkAdjustment *adjustment;

  g_return_val_if_fail(GTK_IS_SPIN_BUTTON(button), FALSE);

  adjustment = gtk_spin_button_get_adjustment(button);
  if (gtk_adjustment_get_value(adjustment) == 0)
  {
    gtk_entry_set_text(GTK_ENTRY(button), UNICODE_INFINITY);
    return TRUE;
  }

  return FALSE;
}

static gboolean
program_separator_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  gboolean separator;

  gtk_tree_model_get(model, iter, 3, &separator, -1);
  return separator;
}

static void
program_changed(GtkComboBox *widget, Alert *alert)
{
  GtkWidget *parent;
  gint active;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GAppInfo *app;
  const gchar *app_name = NULL;
  gchar *program;

  g_return_if_fail(GTK_IS_COMBO_BOX(widget));
  g_return_if_fail(ALARM_PLUGIN_IS_ALERT(alert));

  active = gtk_combo_box_get_active(widget);
  model = gtk_combo_box_get_model(widget);

  if (active == PROGRAM_CHOOSE_FILE)
  {
    parent = gtk_widget_get_toplevel(GTK_WIDGET(widget));
    program = show_program_dialog(alert->builder, parent);
    gtk_combo_box_set_active(widget, select_program_by_cmdline(model, program));
    g_free(program);
    return;
  }

  set_sensitive(alert->builder, active > PROGRAM_CHOOSE_FILE,
                "program-params-box", NULL);

  g_return_if_fail(gtk_combo_box_get_active_iter(widget, &iter));

  gtk_tree_model_get(model, &iter, PROGRAM_DATA, &app, -1);
  if (app != NULL)
  {
    app_name = g_app_info_get_id(app);
    if (app_name == NULL)
      app_name = g_app_info_get_executable(app);
  }

  g_object_set(G_OBJECT(alert), "program", app_name, NULL);
}

static gboolean
program_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GAppInfo *appinfo;

  g_return_val_if_fail(GTK_IS_COMBO_BOX(widget), FALSE);

  model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
  if (gtk_tree_model_get_iter_first(model, &iter))
    do
    {
      gtk_tree_model_get(model, &iter, PROGRAM_DATA, &appinfo, -1);
      if (appinfo != NULL)
        g_object_unref(appinfo);
    }
    while (gtk_tree_model_iter_next(model, &iter));

  return FALSE;
}

static gint
time_spin_input(GtkSpinButton *button, gdouble *new_value, gpointer user_data)
{
  const gchar *time;
  gchar *error = NULL, **parts;
  gint pos = 0;

  g_return_val_if_fail(GTK_IS_SPIN_BUTTON(button), FALSE);

  time = gtk_entry_get_text(GTK_ENTRY(button));
  if (time == NULL)
    return GTK_INPUT_ERROR;

  if (!g_strcmp0(time, UNICODE_INFINITY))
  {
    *new_value = 0;
    return TRUE;
  }

  parts = g_strsplit(time, ":", 3);
  do {
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

static gboolean
time_spin_output(GtkSpinButton *button, gpointer user_data)
{
  GtkAdjustment *adjustment;
  gint value;
  gchar *time;

  g_return_val_if_fail(GTK_IS_SPIN_BUTTON(button), FALSE);

  adjustment = gtk_spin_button_get_adjustment(button);
  value = gtk_adjustment_get_value(adjustment);
  if (value == 0)
    time = g_strdup(UNICODE_INFINITY);
  else
    time = g_strdup_printf("%02u:%02u:%02u", value/3600, value%3600/60, value%60);

  if (g_strcmp0(time, gtk_entry_get_text(GTK_ENTRY(button))))
    gtk_entry_set_text(GTK_ENTRY(button), time);
  g_free(time);

  return TRUE;
}

static gboolean
repeats_to_interval_sensitivity(GBinding *binding, const GValue *from_value,
                                GValue *to_value, gpointer user_data)
{
  g_value_set_boolean(to_value, g_value_get_uint(from_value) != 1);
  return TRUE;
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

  for (prop_id = 1; prop_id < PROP_COUNT; prop_id++)
  {
    pspec = alert_class_props[prop_id];
    xfconf_prop = g_strconcat("/", pspec->name, NULL);
    xfconf_g_property_bind(channel, xfconf_prop, pspec->value_type, alert, pspec->name);
    g_free(xfconf_prop);
  }

  return alert;
}

gboolean
show_alert_box(Alert *alert, XfcePanelPlugin *panel_plugin, GtkContainer *container)
{
  GObject *object, *target, *program;
  GtkWidget *alert_box;
  GList *apps, *app_iter;
  GAppInfo *app;
  guint active_program = PROGRAM_NONE;
  PropertyBinding alert_bindings[] =
  {
    {"notification", "active", "notification", NULL, NULL},
    {"loop-count", "value", "sound-loops", NULL, NULL},
    {"program-options", "text", "program-options", NULL, NULL},
    {"program-runtime", "value", "program-runtime", NULL, NULL},
    {"repeat-count", "value", "repeats", NULL, NULL},
    {"repeat-interval", "value", "interval", NULL, NULL},
    {"repeat-interval", "sensitive", "repeats", repeats_to_interval_sensitivity, NULL},
  };

  alert->builder = alarm_builder_new(panel_plugin, "alert-box", &object,
                                     alert_box_ui, alert_box_ui_length,
                                     NULL);
  g_return_val_if_fail(GTK_IS_BUILDER(alert->builder), FALSE);
  g_object_weak_ref(object, (GWeakNotify) G_CALLBACK(g_steal_pointer), &alert->builder);
  alert_box = GTK_WIDGET(object);

  // Connect alert box to container
  g_object_ref(alert_box);
  gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(alert_box)), alert_box);
  gtk_container_add(container, alert_box);
  g_object_unref(alert_box);

  // Create libcanberra context for playing sounds
  ca_context_create(&alert->context);
  g_object_weak_ref(G_OBJECT(alert_box), (GWeakNotify) G_CALLBACK(clear_context), alert);

  gtk_builder_add_callback_symbols(alert->builder,
      "sound_chooser_selection_changed", G_CALLBACK(sound_chooser_selection_changed),
      "clear_sound_clicked", G_CALLBACK(clear_sound_clicked),
      "play_sound_toggled", G_CALLBACK(play_sound_toggled),
      "loop_count_output", G_CALLBACK(count_spin_output),
      "program_changed", G_CALLBACK(program_changed),
      "program_delete_event", G_CALLBACK(program_delete_event),
      "program_runtime_input", G_CALLBACK(time_spin_input),
      "program_runtime_output", G_CALLBACK(time_spin_output),
      "repeat_count_output", G_CALLBACK(count_spin_output),
      "repeat_interval_input", G_CALLBACK(time_spin_input),
      "repeat_interval_output", G_CALLBACK(time_spin_output),
      NULL);
  gtk_builder_connect_signals(alert->builder, alert);

  program = gtk_builder_get_object(alert->builder, "program");
  g_return_val_if_fail(GTK_IS_COMBO_BOX(program), FALSE);
  gtk_combo_box_set_row_separator_func(GTK_COMBO_BOX(program), program_separator_func, NULL,
                                       NULL);

  /* Fill program list. Positions prefilled from .glade:
   * PROGRAM_NONE:        (None)
   * 1:                   separator
   * PROGRAM_CHOOSE_FILE: file chooser
   * 3:                   separator */
  object = gtk_builder_get_object(alert->builder, "program-store");
  g_return_val_if_fail(GTK_IS_LIST_STORE(object), FALSE);
  apps = g_app_info_get_all();
  apps = g_list_sort(apps, app_order_func);
  app_iter = apps;
  while (app_iter)
  {
    app = app_iter->data;
    if (g_app_info_should_show(app))
    {
      gtk_list_store_insert_with_values(GTK_LIST_STORE(object), NULL, -1,
                                        PROGRAM_DATA, app,
                                        PROGRAM_ICON, g_app_info_get_icon(app),
                                        PROGRAM_NAME, g_app_info_get_display_name(app),
                                        PROGRAM_SEPARATOR, FALSE,
                                        PROGRAM_HAS_ICON, TRUE, -1);
      if ((alert->program != NULL) && !g_strcmp0(alert->program, g_app_info_get_id(app)))
        active_program = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(object), NULL) - 1;
    }
    app_iter = app_iter->next;
  }
  g_list_free(apps);

  if (!exo_str_is_empty(alert->program) && (active_program == PROGRAM_NONE))
    // Program not found by ID, retry searching by command line
    active_program = select_program_by_cmdline(GTK_TREE_MODEL(object), alert->program);
  gtk_combo_box_set_active(GTK_COMBO_BOX(program), active_program);

  // Set widgets according to alert properties
  object = gtk_builder_get_object(alert->builder, "sound-chooser");
  g_return_val_if_fail(GTK_IS_FILE_CHOOSER(object), FALSE);
  if (alert->sound != NULL)
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(object), alert->sound);

  for (guint i = 0; i < sizeof(alert_bindings)/sizeof(alert_bindings[0]); i++)
  {
    target = gtk_builder_get_object(alert->builder, alert_bindings[i].widget_id);
    g_return_val_if_fail(GTK_IS_WIDGET(target), FALSE);
    g_object_bind_property_full(alert, alert_bindings[i].object_prop,
                                target, alert_bindings[i].widget_prop,
                                G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL,
                                alert_bindings[i].transform_to,
                                alert_bindings[i].transform_from,
                                NULL, NULL);
  }

  return TRUE;
}
