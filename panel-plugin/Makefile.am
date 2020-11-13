AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfce4-alarm-plugin\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

plugindir = $(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = \
	libalarm.la

libalarm_built_sources = \
	properties-dialog_ui.h \
	alarm-dialog_ui.h \
	alert-box_ui.h

libalarm_la_SOURCES = \
	$(libalarm_built_sources) \
	alarm.c \
	alarm.h \
	properties-dialog.c \
	properties-dialog.h \
	alarm-dialog.c \
	alarm-dialog.h \
	alert-box.c \
	alert-box.h

libalarm_la_CFLAGS = \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBXFCE4PANEL_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBCANBERRA_CFLAGS) \
	$(PLATFORM_CFLAGS)

libalarm_la_LDFLAGS = \
       -avoid-version \
       -module \
       -no-undefined \
       -export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
       $(PLATFORM_LDFLAGS)

libalarm_la_LIBADD = \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBXFCE4PANEL_LIBS) \
	$(XFCONF_LIBS) \
	$(LIBCANBERRA_LIBS)

#
# .desktop file
#
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_in_files = alarm.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

BUILT_SOURCES = \
	$(libalarm_built_sources)

EXTRA_DIST = \
	properties-dialog.glade \
	alarm-dialog.glade \
	alert-box.glade \
	$(desktop_in_files)

DISTCLEANFILES = \
	$(libalarm_built_sources) \
	$(desktop_DATA)

CLEANFILES = \
	$(desktop_DATA)

properties-dialog_ui.h: properties-dialog.glade
	$(AM_V_GEN) xdt-csource --static --strip-comments --strip-content --name=properties_dialog_ui $< >$@
alarm-dialog_ui.h: alarm-dialog.glade
	$(AM_V_GEN) xdt-csource --static --strip-comments --strip-content --name=alarm_dialog_ui $< >$@
alert-box_ui.h: alert-box.glade
	$(AM_V_GEN) xdt-csource --static --strip-comments --strip-content --name=alert_box_ui $< >$@
# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
