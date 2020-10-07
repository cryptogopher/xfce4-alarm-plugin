AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfce4-alarm-plugin\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

plugindir = $(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = \
	libalarm.la

libalarm_built_sources = \
	plugin-configure-dialog_ui.h

libalarm_la_SOURCES = \
	$(libalarm_built_sources) \
	alarm.c \
	alarm.h

libalarm_la_CFLAGS = \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBXFCE4PANEL_CFLAGS) \
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
	$(LIBXFCE4PANEL_LIBS)

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
	plugin-configure-dialog.glade \
	$(desktop_in_files)

DISTCLEANFILES = \
	$(libalarm_built_sources) \
	$(desktop_DATA)

CLEANFILES = \
	$(desktop_DATA)

plugin-configure-dialog_ui.h: plugin-configure-dialog.glade
	$(AM_V_GEN) xdt-csource --static --strip-comments --strip-content --name=plugin_configure_dialog_ui $< >$@
# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
