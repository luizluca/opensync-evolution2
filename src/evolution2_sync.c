/*
 * evolution2_sync - A plugin for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007 Daniel Friedrich <daniel.friedrich@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */
#include <string.h>

#ifdef HAVE_EDS_VERSION_H
#include <libedataserver/eds-version.h>
#endif /* HAVE_EDS_VERSION_H */

#include <gmodule.h>

#include <opensync/opensync.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-capabilities.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-version.h>

#include "evolution2_ebook.h"
#include "evolution2_ecal.h"
#include "evolution2_sync.h"

void free_osync_evo_calendar(void *data, void* notused)
{
	OSyncEvoCalendar *cal = (OSyncEvoCalendar *)data;
	
	if (cal->calendar) {
		g_object_unref(cal->calendar);
		cal->calendar = NULL;
	}
	if (cal->sink) {
		osync_objtype_sink_unref(cal->sink);
		cal->sink = NULL;
	}
	if (cal->format) {
		osync_objformat_unref(cal->format);
		cal->format = NULL;
	}
}

static void free_env(OSyncEvoEnv *env)
{
	if (env->contact_sink)
		osync_objtype_sink_unref(env->contact_sink);
	if (env->pluginInfo)
		osync_plugin_info_unref(env->pluginInfo);
	if (env->change_id)
		g_free(env->change_id);

	g_list_foreach(env->calendars, free_osync_evo_calendar, NULL);
	g_list_free(env->calendars);

	g_free(env);
}



ESource *evo2_find_source(ESourceList *list, const char *uri)
{
	GSList *g;
	for (g = e_source_list_peek_groups (list); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			ESource *source = E_SOURCE (s->data);
			osync_trace(TRACE_INTERNAL, "Comparing source uri %s and %s", e_source_get_uri(source), uri);
			if (!strcmp(e_source_get_uri(source), uri))
				return source;
			osync_trace(TRACE_INTERNAL, "Comparing source name %s and %s", e_source_peek_name(source), uri);
			if (!strcmp(e_source_peek_name(source), uri))
				return source;
		}
	}
	return NULL;
}

/* In initialize, we get the config for the plugin. Here we also must register
 * all _possible_ objtype sinks. */
static void *evo2_initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, info, error);

	OSyncEvoEnv *env = osync_try_malloc0(sizeof(OSyncEvoEnv), error);
	if (!env)
		goto error;

	env->pluginInfo = osync_plugin_info_ref(info);
		
	osync_trace(TRACE_INTERNAL, "Setting change id: %s", osync_plugin_info_get_groupname(info));

	env->change_id = g_strdup(osync_plugin_info_get_groupname(info));

	osync_trace(TRACE_INTERNAL, "The config: %p", osync_plugin_info_get_config(info));


	g_type_init();

	if (!evo2_ebook_initialize(env, info, error))
		goto error_free_env;
	
	if (!evo2_ecal_initialize(env, info, "event", "vevent20", error))
		goto error_free_env;

	if (!evo2_ecal_initialize(env, info, "todo", "vtodo20", error))
		goto error_free_env;

	if (!evo2_ecal_initialize(env, info, "note", "vjournal", error))
		goto error_free_env;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, env);
	return (void *)env;

error_free_env:
	free_env(env);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void evo2_finalize(void *data)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, data);
	OSyncEvoEnv *env = (OSyncEvoEnv *)data;

	/* cleanup OpenSync stuff */
	if (env->pluginInfo) {
		osync_plugin_info_unref(env->pluginInfo);
		env->pluginInfo = NULL;
	}
	osync_trace(TRACE_INTERNAL, "%s - plugin info cleaned", __func__);

	/* Final cleanup */
	free_env(env);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static char *evo2_determine_version()
{
	char *version = NULL;
#ifdef HAVE_EDS_VERSION
	version = osync_strdup_printf("%i.%i.%i", eds_major_version, eds_minor_version, eds_micro_version);
#else
	version = osync_strdup("Unknown");
#endif /* HAVE_EDS_VERSION */
return version;
}


/* Here we actually tell opensync which sinks are available and their capabilities */

static osync_bool evo2_discover(OSyncPluginInfo *info, void *data, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, error);
	
	OSyncEvoEnv *env = (OSyncEvoEnv *)data;
	
	OSyncList *l, *list = NULL;
	list = osync_plugin_info_get_objtype_sinks(info);
	for (l=list; l; l = l->next) {
		OSyncObjTypeSink *sink = (OSyncObjTypeSink *) l->data;
		osync_assert(sink);

		osync_objtype_sink_set_available(sink, TRUE);
	}
	osync_list_free(list);

	OSyncVersion *version = osync_version_new(error);
	osync_version_set_plugin(version, "Evolution");
	osync_version_set_modelversion(version, "2");
	//osync_version_set_firmwareversion(version, "firmwareversion");
	char *evo_version = evo2_determine_version();
	if (!evo_version) {
		goto error;
	}
	osync_version_set_softwareversion(version, evo_version);
	osync_free(evo_version);
	//osync_version_set_hardwareversion(version, "hardwareversion");
	osync_plugin_info_set_version(info, version);
	osync_version_unref(version);

	OSyncCapabilities *capabilities;
	capabilities = osync_capabilities_new("evo2-caps", error);
	if (!evo2_ebook_discover(env, capabilities, error)) {
		goto error_free_capabilties;
	}
	int i, numcalendars = g_list_length(env->calendars);
	for (i = 0; i < numcalendars; i++) {
		OSyncEvoCalendar *cal = (OSyncEvoCalendar *)g_list_nth_data(env->calendars, i);
		if (!cal || !evo2_ecal_discover(cal, capabilities, error)) {
			goto error_free_capabilties;
		}
	}
	osync_plugin_info_set_capabilities(info, capabilities);
	osync_capabilities_unref(capabilities);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

 error_free_capabilties:
	osync_capabilities_unref(capabilities);
 error:
	osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;
	
	osync_plugin_set_name(plugin, "evo2-sync");
	osync_plugin_set_longname(plugin, "Evolution 2.x");
	osync_plugin_set_description(plugin, "Address book, calendar and task list of Evolution 2");
	
	osync_plugin_set_initialize(plugin, evo2_initialize);
	osync_plugin_set_finalize(plugin, evo2_finalize);
	osync_plugin_set_discover(plugin, evo2_discover);
	osync_plugin_set_start_type(plugin, OSYNC_START_TYPE_PROCESS);
	
	if (!osync_plugin_env_register_plugin(env, plugin, error))
			goto error;

	osync_plugin_unref(plugin);
	
	return TRUE;
	
error:
	osync_trace(TRACE_ERROR, "Unable to register: %s", osync_error_print(error));
	osync_error_unref(error);
	return FALSE;
}

int get_version(void)
{
	return 1;
}

/**
 * Bug 477227 – libebook isn't designed to be loaded and unloaded
 * see: http://bugzilla.gnome.org/show_bug.cgi?id=477227
 * see: http://mail.gnome.org/archives/evolution-hackers/2007-September/msg00027.html
 * also the other EDS client libraries; so prevent unloading this module as the api provides 
 * this facility which is the same as would be the g_module_make_resident that is :
 * "Any future g_module_close() calls on the module will be ignored.".
 */
int dont_free(void)
{
	return 1;
}

const char * g_module_check_init (GModule *module)
{
	g_module_make_resident (module);
	return NULL;
}
