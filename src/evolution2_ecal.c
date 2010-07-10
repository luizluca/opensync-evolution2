/*
 * evolution2_sync - A plugin for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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
#include <glib.h>

#include <opensync/opensync.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-plugin.h>

#include "evolution2_capabilities.h"
#include "evolution2_ecal.h"

ECal *evo2_ecal_open_cal(const char *path, ECalSourceType source_type, OSyncError **error)
{
	ECal *calendar = NULL;
	GError *gerror = NULL;
        ESourceList *sources = NULL;
        ESource *source = NULL;

	if (!path) {
                osync_error_set(error, OSYNC_ERROR_GENERIC, "No path set");
                goto error;
        }

        if (strcmp(path, "default")) {
                if (!e_cal_get_sources(&sources,source_type, &gerror)) {
                        osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get sources for calendar: %s", gerror ? gerror->message : "None");
                        goto error;
                }
                
                if (!(source = evo2_find_source(sources, path))) {
                        osync_error_set(error, OSYNC_ERROR_GENERIC, "Error finding source \"%s\"", path);
                        goto error;
                }
 
                if (!(calendar = e_cal_new(source, source_type))) {
                        osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to create new calendar");
			goto error;
		}

		if(!e_cal_open(calendar, FALSE, &gerror)) {
                        osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to open calendar: %s", gerror ? gerror->message : "None");
                        goto error_free_event;
                }
        } else {
                osync_trace(TRACE_INTERNAL, "Opening default calendar\n");
                if (!e_cal_open_default(&calendar, source_type, NULL, NULL, &gerror)) {
                        osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to open default calendar: %s", gerror ? gerror->message : "None");
                        goto error_free_event;
                }
        }
	osync_trace(TRACE_EXIT, "%s", __func__);
	return calendar;
	
 error_free_event:
	g_object_unref(calendar);
 error:
	if (gerror)
		g_clear_error(&gerror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void evo2_ecal_connect(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
        OSyncError *error = NULL;
       
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sink, info, ctx, userdata);
 	OSyncEvoCalendar * evo_cal = (OSyncEvoCalendar *)userdata;

	if (!(evo_cal->calendar = evo2_ecal_open_cal(evo_cal->uri, evo_cal->source_type, &error))) {
		goto error;
	}

	OSyncSinkStateDB *state_db = osync_objtype_sink_get_state_db(sink);
	osync_bool state_match;
	if (!state_db) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Anchor missing for objtype \"%s\"", osync_objtype_sink_get_name(sink));
		goto error_free_cal;
	}
	if (!osync_sink_state_equal(state_db, evo_cal->uri_key, evo_cal->uri, &state_match, &error)) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Anchor comparison failed for objtype \"%s\"", osync_objtype_sink_get_name(sink));
		goto error_free_cal;
	}
	if (!state_match) {
		osync_trace(TRACE_INTERNAL, "ECal slow sync, due to anchor mismatch for objtype \"%s\"", osync_objtype_sink_get_name(sink));
		osync_context_report_slowsync(ctx);
	}

        osync_context_report_success(ctx);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return;

 error_free_cal:
	g_object_unref(evo_cal->calendar);
	evo_cal->calendar = NULL;
error:
	osync_context_report_osyncerror(ctx, error);
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
        osync_error_unref(&error);
}

static void evo2_ecal_disconnect(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sink, info, ctx, userdata);

	OSyncEvoCalendar * evo_cal = (OSyncEvoCalendar *)userdata;

        if (evo_cal->calendar) {
                g_object_unref(evo_cal->calendar);
                evo_cal->calendar = NULL;
        }

        osync_context_report_success(ctx);

        osync_trace(TRACE_EXIT, "%s", __func__);
}

static void evo2_ecal_sync_done(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, ctx);

	OSyncError *error = NULL;
	GError *gerror = NULL;

	OSyncEvoCalendar * evo_cal = (OSyncEvoCalendar *)userdata;

	OSyncSinkStateDB *state_db = osync_objtype_sink_get_state_db(sink);
	if (!state_db) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "State database missing for objtype \"%s\"", osync_objtype_sink_get_name(sink));
		goto error;
	}
	if (!osync_sink_state_set(state_db, evo_cal->uri_key, evo_cal->uri, &error))
		goto error;

        GList *changes = NULL;
        if (!e_cal_get_changes(evo_cal->calendar, evo_cal->change_id, &changes, &gerror)) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to update %s ECal time of last sync: %s", evo_cal->objtype, gerror ? gerror->message : "None");
		g_clear_error(&gerror);
		goto error;
	}

	e_cal_free_change_list(changes);
        osync_context_report_success(ctx);
        
        osync_trace(TRACE_EXIT, "%s", __func__);
	return;

 error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

void evo2_ecal_report_change(OSyncContext *ctx, OSyncObjFormat *format, char *data, unsigned int size, const char *uid, OSyncChangeType changetype)
{
        OSyncError *error = NULL;

        OSyncChange *change = osync_change_new(&error);
        if (!change) {
                osync_context_report_osyncwarning(ctx, error);
                osync_error_unref(&error);
                return;
        }

        osync_change_set_uid(change, uid);
        osync_change_set_changetype(change, changetype);

        OSyncData *odata = osync_data_new(data, size, format, &error);
        if (!odata) {
                osync_change_unref(change);
                osync_context_report_osyncwarning(ctx, error);
                osync_error_unref(&error);
                return;
        }

        osync_change_set_data(change, odata);
        osync_data_unref(odata);

        osync_context_report_change(ctx, change);

        osync_change_unref(change);
}


static void evo2_ecal_get_changes(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, osync_bool slow_sync, void *userdata)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %s, %p)", __func__, sink, info, ctx, slow_sync ? "TRUE" : "FALSE", userdata);
        OSyncError *error = NULL;

        GList *changes = NULL;
        ECalChange *ecc = NULL;
        GList *l = NULL;
        char *data = NULL;
        const char *uid = NULL;
        int datasize = 0;
        GError *gerror = NULL;

	OSyncEvoCalendar * evo_cal = (OSyncEvoCalendar *)userdata;

        if (slow_sync == FALSE) {
                osync_trace(TRACE_INTERNAL, "No slow_sync for %s", evo_cal->objtype);
                if (!e_cal_get_changes(evo_cal->calendar, evo_cal->change_id, &changes, &gerror)) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to open changed %s entries: %s", evo_cal->objtype, gerror ? gerror->message : "None");
                        goto error;
                }
                osync_trace(TRACE_INTERNAL, "Found %i changes for change-ID %s", g_list_length(changes), evo_cal->change_id);

                for (l = changes; l; l = l->next) {
                        ecc = (ECalChange *)l->data;
			e_cal_component_get_uid(ecc->comp, &uid);
			e_cal_component_commit_sequence (ecc->comp);
			e_cal_component_strip_errors(ecc->comp);
			switch (ecc->type) {
				case E_CAL_CHANGE_ADDED:
					data = e_cal_get_component_as_string(evo_cal->calendar, e_cal_component_get_icalcomponent(ecc->comp));
					datasize = strlen(data) + 1;
					evo2_ecal_report_change(ctx, evo_cal->format, data, datasize, uid, OSYNC_CHANGE_TYPE_ADDED);
					break;
				case E_CAL_CHANGE_MODIFIED:
					data = e_cal_get_component_as_string(evo_cal->calendar, e_cal_component_get_icalcomponent(ecc->comp));
					datasize = strlen(data) + 1;
					evo2_ecal_report_change(ctx, evo_cal->format, data, datasize, uid, OSYNC_CHANGE_TYPE_MODIFIED);
					break;
				case E_CAL_CHANGE_DELETED:
					evo2_ecal_report_change(ctx, evo_cal->format, NULL, 0, uid, OSYNC_CHANGE_TYPE_DELETED);
					break;
			}
                }
        } else {
                osync_trace(TRACE_INTERNAL, "slow_sync for %s", evo_cal->objtype);
	        if (!e_cal_get_object_list_as_comp (evo_cal->calendar, "(has-start?)", &changes, &gerror)) {
                        osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to get %s changes: %s",  evo_cal->objtype, gerror ? gerror->message : "None");
                        goto error;
        	}
		for (l = changes; l; l = l->next) {
			ECalComponent *comp = E_CAL_COMPONENT (l->data);
			data = e_cal_get_component_as_string(evo_cal->calendar, e_cal_component_get_icalcomponent(comp));
			e_cal_component_get_uid(comp, &uid);
			datasize = strlen(data) + 1;
			evo2_ecal_report_change(ctx, evo_cal->format, data, datasize, uid, OSYNC_CHANGE_TYPE_ADDED);
			g_object_unref (comp);
		}
	}

        osync_context_report_success(ctx);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return;

error:
        if (gerror)
                g_clear_error(&gerror);
        osync_context_report_osyncerror(ctx, error);
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
        osync_error_unref(&error);
}

static void evo2_ecal_modify(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change, void *userdata)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, sink, info, ctx, change, userdata);

        const char *uid = osync_change_get_uid(change);
	icalcomponent *icomp = NULL;
	char *returnuid = NULL;
        GError *gerror = NULL;
        OSyncError *error = NULL;
        OSyncData *odata = NULL;
        char *plain = NULL;

	OSyncEvoCalendar * evo_cal = (OSyncEvoCalendar *)userdata;

        switch (osync_change_get_changetype(change)) {
                case OSYNC_CHANGE_TYPE_DELETED:
                        if (!e_cal_remove_object(evo_cal->calendar, uid, &gerror)) {
                                osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to delete %s: %s", evo_cal->objtype, gerror ? gerror->message : "None");
                                goto error;
                        }
                        break;
                case OSYNC_CHANGE_TYPE_ADDED:
                        odata = osync_change_get_data(change);
                        osync_data_get_data(odata, &plain, NULL);
			icomp = icalcomponent_new_from_string(plain);
			if (!icomp) {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to convert %s", evo_cal->objtype);
				goto error;
			}
			
			icomp = icalcomponent_get_first_component (icomp, evo_cal->ical_component);
			if (!icomp) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to get %s", evo_cal->objtype);
				goto error;
			}
			
			if (!e_cal_create_object(evo_cal->calendar, icomp, &returnuid, &gerror)) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to create %s: %s", evo_cal->objtype, gerror ? gerror->message : "None");
				goto error;
			}
			osync_change_set_uid(change, returnuid);
                        break;
                case OSYNC_CHANGE_TYPE_MODIFIED:
                        odata = osync_change_get_data(change);
                        osync_data_get_data(odata, &plain, NULL);

			icomp = icalcomponent_new_from_string(plain);
			if (!icomp) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to convert %s", evo_cal->objtype);
				goto error;
			}
			
			icomp = icalcomponent_get_first_component (icomp, evo_cal->ical_component);
			if (!icomp) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to get %s", evo_cal->objtype);
				goto error;
			}
			
			icalcomponent_set_uid (icomp, uid);
			if (!e_cal_modify_object(evo_cal->calendar, icomp, CALOBJ_MOD_ALL, &gerror)) {
				osync_trace(TRACE_INTERNAL, "unable to mod %s: %s", evo_cal->objtype, gerror ? gerror->message : "None");
				g_clear_error(&gerror);
				if (!e_cal_create_object(evo_cal->calendar, icomp, &returnuid, &gerror)) {
					osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to create %s: %s", evo_cal->objtype, gerror ? gerror->message : "None");
					goto error;
				}
			}
                        break;
                default:
                        printf("Error\n");
        }

        osync_context_report_success(ctx);

        osync_trace(TRACE_EXIT, "%s", __func__);
        return;

error:
        if (gerror)
                g_clear_error(&gerror);
        osync_context_report_osyncerror(ctx, error);
        osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
        osync_error_unref(&error);
}

osync_bool evo2_ecal_discover(OSyncEvoCalendar *evo_cal, OSyncCapabilities *caps, OSyncError **error)
{
	ECal *cal = NULL;
	GError *gerror = NULL;
	gboolean read_only;
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, evo_cal, caps, error);

	if (evo_cal->sink) {
		if (!(cal = evo2_ecal_open_cal(evo_cal->uri, evo_cal->source_type, error))) {
			goto error;
		}
		if (!e_cal_is_read_only(cal, &read_only, &gerror)) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not determine if source was read only: %s", gerror ? gerror->message : "None");
			goto error_free_cal;
		}
		g_object_unref(cal);

		osync_objtype_sink_set_write(evo_cal->sink, !read_only);
		osync_trace(TRACE_INTERNAL, "Set sink write status to %s", read_only ? "FALSE" : "TRUE");

	}
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

 error_free_cal:
	g_object_unref(cal);
 error:
	if (gerror)
		g_clear_error(&gerror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
        return FALSE;
}

osync_bool evo2_ecal_initialize(OSyncEvoEnv *env, OSyncPluginInfo *info, const char *objtype, const char *required_format, OSyncError **error)
{
	char *uri_key;

	osync_assert(env);
	osync_assert(info);
	osync_assert(objtype);
	osync_assert(required_format);

	OSyncObjTypeSink *sink = osync_plugin_info_find_objtype(info, objtype);
        if (!sink)
                return TRUE;
        osync_bool sinkEnabled = osync_objtype_sink_is_enabled(sink);
        osync_trace(TRACE_INTERNAL, "%s: enabled => %d", __func__, sinkEnabled);
        if (!sinkEnabled)
                return TRUE;

        osync_objtype_sink_set_connect_func(sink, evo2_ecal_connect);
        osync_objtype_sink_set_disconnect_func(sink, evo2_ecal_disconnect);
        osync_objtype_sink_set_get_changes_func(sink, evo2_ecal_get_changes);
        osync_objtype_sink_set_commit_func(sink, evo2_ecal_modify);
        osync_objtype_sink_set_sync_done_func(sink, evo2_ecal_sync_done);

	osync_objtype_sink_enable_state_db(sink, TRUE);

	OSyncEvoCalendar *cal = osync_try_malloc0(sizeof(OSyncEvoCalendar), error);
	if (!cal) {
		return FALSE;
	}
	cal->objtype = objtype;
	cal->change_id = env->change_id;

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
        OSyncPluginResource *resource = osync_plugin_config_find_active_resource(config, objtype);

		const int size = strlen(STR_URI_KEY) + strlen(objtype) + 1;

		uri_key = malloc(size * sizeof(char));

		snprintf(uri_key, size, "%s%s", STR_URI_KEY, objtype);

		cal->uri_key = uri_key;
        cal->uri = osync_plugin_resource_get_url(resource);
        if(!cal->uri) {
                osync_error_set(error,OSYNC_ERROR_GENERIC, "%s url not set", objtype);
                return FALSE;
        }
        OSyncList *objformatsinks = osync_plugin_resource_get_objformat_sinks(resource);
        osync_bool hasObjFormat = FALSE;
        OSyncList *r;
        for(r = objformatsinks;r;r = r->next) {
                OSyncObjFormatSink *objformatsink = r->data;
                if(!strcmp(required_format, osync_objformat_sink_get_objformat(objformatsink))) { hasObjFormat = TRUE; break;}
        }
	osync_list_free(objformatsinks);
        if (!hasObjFormat) {
                osync_error_set(error, OSYNC_ERROR_GENERIC, "Format %s not set.", required_format);
                return FALSE;
        }

        OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
        cal->format = osync_format_env_find_objformat(formatenv, required_format);
        assert(cal->format);
	osync_objformat_ref(cal->format);
	
	if (strcmp(cal->objtype, "event") == 0) {
		cal->source_type = E_CAL_SOURCE_TYPE_EVENT;
		cal->ical_component = ICAL_VEVENT_COMPONENT;
	} else if (strcmp(cal->objtype, "todo") == 0) {
		cal->source_type = E_CAL_SOURCE_TYPE_TODO;
		cal->ical_component = ICAL_VTODO_COMPONENT;
	} else if (strcmp(cal->objtype, "note") == 0) {
		cal->source_type = E_CAL_SOURCE_TYPE_JOURNAL;
		cal->ical_component = ICAL_VJOURNAL_COMPONENT;
	} else {
		return FALSE;
	}

        cal->sink = osync_objtype_sink_ref(sink);

        osync_objtype_sink_set_userdata(cal->sink, cal);

	env->calendars = g_list_append(env->calendars, cal);
	return TRUE;
}
